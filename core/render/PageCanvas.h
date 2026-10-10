#ifndef DIZITABFORGE_CORE_RENDER_PAGECANVAS_H
#define DIZITABFORGE_CORE_RENDER_PAGECANVAS_H

#include <QSizeF>
#include <QString>
#include <QStringList>

#include <memory>

namespace dizi {

class PageCanvas {
public:
    virtual ~PageCanvas() = default;

    virtual bool prepare(const QStringList &texts, const QString &outputPath, QSizeF pageSize,
                         QString &error) = 0;

    virtual void addPage() = 0;

    virtual void setFontSize(double points) = 0;

    virtual void setLineWidth(double points) = 0;

    virtual void drawText(double left, double baseline, const QString &text) = 0;

    virtual void drawHole(double centerX, double centerY, double radius, bool filled) = 0;

    virtual bool finish(QString &error) = 0;
};

enum class CanvasKind {
    QtPdf,
    Handwritten
};

std::unique_ptr<PageCanvas> makePageCanvas(CanvasKind kind);

}

#endif
