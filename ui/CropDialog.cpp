#include "ui/CropDialog.h"

#include "ui_CropDialog.h"

#include <QPalette>
#include <QDialog>
#include <QEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QShowEvent>

#include "core/util/ImageUtil.h"
#include "core/util/PathUtil.h"

namespace {

constexpr int kPreviewBound = 520;
constexpr int kMinCropSide = 8;

QPoint pixmapOrigin(const QLabel *label) {
    const QSize pixmap = label->pixmap().size();
    const QRect area = label->contentsRect();
    const Qt::Alignment alignment = label->alignment();
    int x = area.x();
    int y = area.y();
    if (alignment & Qt::AlignHCenter) {
        x += (area.width() - pixmap.width()) / 2;
    } else if (alignment & Qt::AlignRight) {
        x += area.width() - pixmap.width();
    }
    if (alignment & Qt::AlignVCenter) {
        y += (area.height() - pixmap.height()) / 2;
    } else if (alignment & Qt::AlignBottom) {
        y += area.height() - pixmap.height();
    }
    return QPoint(x, y);
}

QSize previewBound(const QLabel *label) {
    const QSize available = label->contentsRect().size();
    if (available.width() < kMinCropSide || available.height() < kMinCropSide) {
        return QSize(kPreviewBound, kPreviewBound);
    }
    return available;
}

}

CropDialog::CropDialog(const QString &imagePath, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::CropDialog()),
      m_sourcePath(imagePath) {
    ui->setupUi(this);
    connect(ui->cropButton, &QPushButton::clicked, this, &CropDialog::applyCrop);
    connect(ui->resetButton, &QPushButton::clicked, this, &CropDialog::resetSelection);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    ui->imageLabel->installEventFilter(this);
    QString error;
    if (!image_util::loadImage(imagePath, m_source, error)) {
        showError(error);
        return;
    }
    ui->sizeLabel->setText(QStringLiteral("Source: %1 x %2")
                                   .arg(m_source.width())
                                   .arg(m_source.height()));
    refreshPreview();
}

CropDialog::~CropDialog() {
    delete ui;
}

QString CropDialog::resultPath() const {
    return m_resultPath;
}

QRect CropDialog::selectionInWidget() const {
    const QRect selection = image_util::normalizedRect(m_anchor, m_current);
    return image_util::clampRectTo(selection, ui->imageLabel->pixmap().size());
}

QRect CropDialog::selectionInImage() const {
    return image_util::mapToImage(selectionInWidget(), ui->imageLabel->pixmap().size(),
                                  m_source.size());
}

void CropDialog::setAnchor(const QPoint &position) {
    m_anchor = position;
    m_current = position;
    m_dragging = true;
}

void CropDialog::setCurrent(const QPoint &position) {
    m_current = position;
    refreshPreview();
}

QPoint CropDialog::toPreviewPoint(const QPoint &labelPoint) const {
    return labelPoint - pixmapOrigin(ui->imageLabel);
}

bool CropDialog::eventFilter(QObject *watched, QEvent *event) {
    if (watched != ui->imageLabel) {
        return QDialog::eventFilter(watched, event);
    }
    if (event->type() == QEvent::MouseButtonPress) {
        setAnchor(toPreviewPoint(static_cast<QMouseEvent *>(event)->pos()));
        return true;
    }
    if (event->type() == QEvent::MouseMove && m_dragging) {
        setCurrent(toPreviewPoint(static_cast<QMouseEvent *>(event)->pos()));
        return true;
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        setCurrent(toPreviewPoint(static_cast<QMouseEvent *>(event)->pos()));
        m_dragging = false;
        return true;
    }
    return QDialog::eventFilter(watched, event);
}

void CropDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    refreshPreview();
}

void CropDialog::refreshPreview() {
    if (m_source.isNull()) {
        return;
    }
    const QSize fitted = image_util::fitSize(m_source.size(), previewBound(ui->imageLabel));
    if (fitted.isEmpty()) {
        return;
    }
    m_preview = m_source.scaled(fitted, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap canvas = QPixmap::fromImage(m_preview);
    const QRect selection = selectionInWidget();
    if (!selection.isEmpty()) {
        QPainter painter(&canvas);
        painter.setPen(QPen(palette().color(QPalette::Highlight), 2));
        painter.drawRect(selection.adjusted(0, 0, -1, -1));
        painter.end();
    }
    ui->imageLabel->setPixmap(canvas);
    const QRect region = selectionInImage();
    ui->selectionLabel->setText(QStringLiteral("Crop: %1 x %2")
                                        .arg(region.width())
                                        .arg(region.height()));
}

void CropDialog::resetSelection() {
    if (m_preview.isNull()) {
        return;
    }
    const QSize canvas = ui->imageLabel->pixmap().size();
    m_anchor = QPoint(0, 0);
    m_current = QPoint(canvas.width(), canvas.height());
    m_dragging = false;
    refreshPreview();
}

void CropDialog::applyCrop() {
    if (m_source.isNull()) {
        showError(QStringLiteral("No image is loaded."));
        return;
    }
    const QRect region = selectionInImage();
    if (region.width() < kMinCropSide || region.height() < kMinCropSide) {
        showError(QStringLiteral("Drag a larger area before cropping."));
        return;
    }
    const QString target = path_util::siblingFilePath(m_sourcePath, QStringLiteral("crop"));
    QString error;
    if (!image_util::savePng(m_source.copy(region), target, error)) {
        showError(error);
        return;
    }
    m_resultPath = target;
    accept();
}

void CropDialog::showError(const QString &message) {
    ui->statusLabel->setText(message);
}
