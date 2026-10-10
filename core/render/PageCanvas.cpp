#include "core/render/PageCanvas.h"

#include "core/render/QtPdfCanvas.h"
#include "core/render/VectorPdfCanvas.h"

namespace dizi {

std::unique_ptr<PageCanvas> makePageCanvas(CanvasKind kind) {
    if (kind == CanvasKind::Handwritten) {
        return std::make_unique<VectorPdfCanvas>();
    }
    return std::make_unique<QtPdfCanvas>();
}

}
