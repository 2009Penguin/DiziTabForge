#ifndef DIZITABFORGE_CORE_RENDER_PDFRENDERER_H
#define DIZITABFORGE_CORE_RENDER_PDFRENDERER_H

#include <QString>

#include <vector>

#include "core/model/Note.h"
#include "core/render/FingerTable.h"
#include "core/render/PageCanvas.h"

namespace dizi {

class PdfRenderer {
public:
    explicit PdfRenderer(CanvasKind kind = CanvasKind::QtPdf);

    bool render(const std::vector<Score> &scores, const QString &outputPath, QString &error);

private:
    FingerTable m_fingerTable;
    CanvasKind m_kind = CanvasKind::QtPdf;
};

}

#endif
