#ifndef DIZITABFORGE_CORE_RENDER_VECTORPDFCANVAS_H
#define DIZITABFORGE_CORE_RENDER_VECTORPDFCANVAS_H

#include <QString>
#include <QStringList>

#include <memory>

#include "core/render/PageCanvas.h"

namespace dizi {

class PdfWriter;

class VectorPdfCanvas : public PageCanvas {
public:
    VectorPdfCanvas();

    ~VectorPdfCanvas() override;

    bool prepare(const QStringList &texts, const QString &outputPath, QSizeF pageSize,
                 QString &error) override;

    void addPage() override;

    void setFontSize(double points) override;

    void setLineWidth(double points) override;

    void drawText(double left, double baseline, const QString &text) override;

    void drawHole(double centerX, double centerY, double radius, bool filled) override;

    bool finish(QString &error) override;

private:
    std::unique_ptr<PdfWriter> m_writer;
    QString m_outputPath;
    double m_pageHeight = 0.0;
    double m_lineWidth = 0.6;
};

}

#endif
