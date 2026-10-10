#ifndef DIZITABFORGE_CORE_RENDER_QTPDFCANVAS_H
#define DIZITABFORGE_CORE_RENDER_QTPDFCANVAS_H

#include <QColor>
#include <QString>
#include <QStringList>

#include <memory>

#include "core/render/PageCanvas.h"

class QPainter;
class QPdfWriter;

namespace dizi {

class QtPdfCanvas : public PageCanvas {
public:
    QtPdfCanvas();

    ~QtPdfCanvas() override;

    bool prepare(const QStringList &texts, const QString &outputPath, QSizeF pageSize,
                 QString &error) override;

    void addPage() override;

    void setFontSize(double points) override;

    void setLineWidth(double points) override;

    void drawText(double left, double baseline, const QString &text) override;

    void drawHole(double centerX, double centerY, double radius, bool filled) override;

    bool finish(QString &error) override;

private:
    void applyFont();

    std::unique_ptr<QPdfWriter> m_writer;
    std::unique_ptr<QPainter> m_painter;
    QString m_outputPath;
    QString m_family;
    QColor m_color{0, 0, 0};
    double m_fontSize = 12.0;
    double m_lineWidth = 0.6;
    bool m_started = false;
};

}

#endif
