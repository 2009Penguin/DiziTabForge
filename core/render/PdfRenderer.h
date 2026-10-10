#ifndef DIZITABFORGE_CORE_RENDER_PDFRENDERER_H
#define DIZITABFORGE_CORE_RENDER_PDFRENDERER_H

#include <QString>
#include <vector>

#include "core/model/Note.h"
#include "core/render/FingerTable.h"

namespace dizi {

struct ScorePage {
    QString caption;
    Score notes;
};

class PdfRenderer {
public:
    bool render(const std::vector<ScorePage> &pages, const QString &outputPath, QString &error);

private:
    FingerTable m_fingerTable;
};

}

#endif
