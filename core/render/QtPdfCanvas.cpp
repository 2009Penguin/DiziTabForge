#include "core/render/QtPdfCanvas.h"

#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPen>

namespace dizi {

namespace {

constexpr int kRenderResolution = 72;
constexpr double kTrailingMarginLimit = 0.0;

QString loadBundledFamily() {
    const QString resource = QStringLiteral(":/fonts/WenYuanSerifSC-Regular.ttf");
    const int identifier = QFontDatabase::addApplicationFont(resource);
    if (identifier >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(identifier);
        if (!families.isEmpty()) {
            return families.first();
        }
    }
    const QStringList candidates{QStringLiteral("Microsoft YaHei"), QStringLiteral("SimHei"),
                                 QStringLiteral("DengXian"), QStringLiteral("SimSun")};
    const QStringList installed = QFontDatabase::families();
    for (const QString &name : candidates) {
        if (installed.contains(name)) {
            return name;
        }
    }
    return QString();
}

QString bundledFamily() {
    static const QString family = loadBundledFamily();
    return family;
}

}

QtPdfCanvas::QtPdfCanvas() = default;

QtPdfCanvas::~QtPdfCanvas() = default;

bool QtPdfCanvas::prepare(const QStringList &texts, const QString &outputPath, QSizeF pageSize,
                          QString &error) {
    Q_UNUSED(texts)
    m_outputPath = outputPath;
    m_family = bundledFamily();
    m_writer = std::make_unique<QPdfWriter>(outputPath);
    m_writer->setPageSize(QPageSize(pageSize, QPageSize::Point));
    m_writer->setPageMargins(QMarginsF(kTrailingMarginLimit, kTrailingMarginLimit,
                                       kTrailingMarginLimit, kTrailingMarginLimit),
                             QPageLayout::Point);
    m_writer->setResolution(kRenderResolution);
    m_writer->setTitle(QStringLiteral("DiziTabForge"));
    m_writer->setCreator(QStringLiteral("DiziTabForge"));
    m_painter = std::make_unique<QPainter>(m_writer.get());
    if (!m_painter->isActive()) {
        error = QStringLiteral("无法创建 PDF 绘制设备：%1").arg(outputPath);
        m_painter.reset();
        m_writer.reset();
        return false;
    }
    m_painter->setRenderHint(QPainter::Antialiasing, true);
    m_painter->setRenderHint(QPainter::TextAntialiasing, true);
    applyFont();
    error.clear();
    return true;
}

void QtPdfCanvas::applyFont() {
    if (m_painter == nullptr) {
        return;
    }
    QFont font = m_family.isEmpty() ? QFont() : QFont(m_family);
    font.setPointSizeF(m_fontSize);
    font.setStyleStrategy(QFont::PreferOutline);
    m_painter->setFont(font);
}

void QtPdfCanvas::addPage() {
    if (m_writer == nullptr) {
        return;
    }
    if (m_started) {
        m_writer->newPage();
    }
    m_started = true;
}

void QtPdfCanvas::setFontSize(double points) {
    if (points <= 0.0 || points == m_fontSize) {
        return;
    }
    m_fontSize = points;
    applyFont();
}

void QtPdfCanvas::setLineWidth(double points) {
    if (points > 0.0) {
        m_lineWidth = points;
    }
}

void QtPdfCanvas::drawText(double left, double baseline, const QString &text) {
    if (m_painter == nullptr || text.isEmpty()) {
        return;
    }
    m_painter->setPen(m_color);
    m_painter->drawText(QPointF(left, baseline), text);
}

void QtPdfCanvas::drawHole(double centerX, double centerY, double radius, bool filled) {
    if (m_painter == nullptr || radius <= 0.0) {
        return;
    }
    m_painter->setPen(QPen(m_color, m_lineWidth));
    m_painter->setBrush(filled ? QBrush(m_color) : Qt::NoBrush);
    m_painter->drawEllipse(QPointF(centerX, centerY), radius, radius);
}

bool QtPdfCanvas::finish(QString &error) {
    if (m_painter != nullptr) {
        m_painter->end();
    }
    m_painter.reset();
    m_writer.reset();
    const QFileInfo info(m_outputPath);
    if (!info.exists() || info.size() <= 0) {
        error = QStringLiteral("PDF 没有写出任何内容：%1").arg(m_outputPath);
        return false;
    }
    error.clear();
    return true;
}

}
