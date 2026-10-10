#include "core/render/PdfRenderer.h"

#include <vector>

#include "core/render/NoteFormatter.h"
#include "core/render/PdfWriter.h"

namespace dizi {

namespace {

constexpr double kPageWidth = 595.0;
constexpr double kPageHeight = 842.0;
constexpr double kMargin = 45.0;
constexpr double kHeaderBaseline = kPageHeight - 60.0;
constexpr double kFirstBaseline = kPageHeight - 105.0;
constexpr double kLineSpacing = 78.0;
constexpr double kNoteSpacing = 70.0;
constexpr int kNotesPerLine = 7;
constexpr int kLinesPerPage = 9;
constexpr double kNoteFontSize = 18.0;
constexpr double kLabelFontSize = 8.0;
constexpr double kHeaderFontSize = 12.0;
constexpr double kHoleRadius = 3.4;
constexpr double kHoleSpacing = 9.5;
constexpr double kLabelDrop = 58.0;
constexpr double kDurationDrop = 20.0;
constexpr double kHoleTopDrop = 34.0;
constexpr double kHoleCenterOffset = 8.0;
constexpr double kLabelOffset = -2.0;
constexpr double kLineWidth = 0.7;
constexpr std::size_t kCaptionLimit = 64;

int notesPerDocument() {
    return kNotesPerLine * kLinesPerPage;
}

std::string truncateText(const std::string &text, std::size_t limit) {
    if (text.size() <= limit) {
        return text;
    }
    return text.substr(0, limit) + "...";
}

void drawHoles(PdfWriter &writer, double centerX, double topY, const Fingering &fingering) {
    for (std::size_t index = 0; index < fingering.holes.size(); ++index) {
        const double y = topY - static_cast<double>(index) * kHoleSpacing;
        writer.drawCircle(centerX, y, kHoleRadius, fingering.holes[index]);
    }
}

void drawNoteBody(PdfWriter &writer, const Note &note, const Fingering &fingering, double x,
                  double baseline) {
    writer.setFontSize(kNoteFontSize);
    writer.drawText(x, baseline, note_format::asciiGlyph(note));
    writer.drawText(x, baseline - kDurationDrop, note_format::durationLabel(note));
    drawHoles(writer, x + kHoleCenterOffset, baseline - kHoleTopDrop, fingering);
    writer.setFontSize(kLabelFontSize);
    writer.drawText(x + kLabelOffset, baseline - kLabelDrop,
                    truncateText(fingering.pitchName, 4));
}

void drawNoteAt(PdfWriter &writer, const FingerTable &table, const Note &note, int line,
                int column) {
    const double x = kMargin + static_cast<double>(column) * kNoteSpacing;
    const double baseline = kFirstBaseline - static_cast<double>(line) * kLineSpacing;
    drawNoteBody(writer, note, table.lookup(note), x, baseline);
}

void drawCaption(PdfWriter &writer, const ScorePage &page, std::size_t index) {
    writer.setFontSize(kHeaderFontSize);
    writer.drawText(kMargin, kHeaderBaseline,
                    "Part " + std::to_string(index + 1) + ": " +
                            truncateText(page.caption.toStdString(), kCaptionLimit));
}

void drawPageFooter(PdfWriter &writer, const FingerTable &table, int partNumber, int pageNumber) {
    writer.setFontSize(kLabelFontSize);
    writer.drawText(kMargin, kMargin, "Key: " + table.keyName() + "   part " +
                                             std::to_string(partNumber) + "   page " +
                                             std::to_string(pageNumber));
}

void drawScoreOnPage(PdfWriter &writer, const FingerTable &table, const ScorePage &page,
                     std::size_t partIndex, std::size_t start) {
    drawCaption(writer, page, partIndex);
    for (int offset = 0; offset < notesPerDocument(); ++offset) {
        const std::size_t index = start + static_cast<std::size_t>(offset);
        if (index >= page.notes.size()) {
            break;
        }
        drawNoteAt(writer, table, page.notes[index], offset / kNotesPerLine,
                   offset % kNotesPerLine);
    }
}

void renderPart(PdfWriter &writer, const FingerTable &table, const ScorePage &page,
                std::size_t partIndex) {
    const std::size_t total = page.notes.empty() ? 1 : page.notes.size();
    const std::size_t perPage = static_cast<std::size_t>(notesPerDocument());
    const std::size_t pageCount = (total + perPage - 1) / perPage;
    for (std::size_t pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
        writer.addPage();
        drawScoreOnPage(writer, table, page, partIndex, pageIndex * perPage);
        drawPageFooter(writer, table, static_cast<int>(partIndex + 1),
                       static_cast<int>(pageIndex + 1));
    }
}

}

bool PdfRenderer::render(const std::vector<ScorePage> &pages, const QString &outputPath,
                         QString &error) {
    if (pages.empty()) {
        error = QStringLiteral("没有可渲染的内容：没有识别到任何简谱。");
        return false;
    }
    if (outputPath.trimmed().isEmpty()) {
        error = QStringLiteral("输出 PDF 路径为空。");
        return false;
    }
    PdfWriter writer(kPageWidth, kPageHeight);
    writer.setLineWidth(kLineWidth);
    for (std::size_t index = 0; index < pages.size(); ++index) {
        renderPart(writer, m_fingerTable, pages[index], index);
    }
    std::string problem;
    if (!writer.save(outputPath.toStdString(), problem)) {
        error = QStringLiteral("写入 PDF 失败：%1").arg(QString::fromStdString(problem));
        return false;
    }
    error.clear();
    return true;
}

}
