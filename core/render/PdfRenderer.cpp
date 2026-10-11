#include "core/render/PdfRenderer.h"

#include <QSizeF>
#include <QStringList>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "core/render/NoteFormatter.h"

namespace dizi {

namespace {

constexpr double kPageWidth = 595.0;
constexpr double kPageHeight = 842.0;
constexpr double kMargin = 45.0;
constexpr double kFooterBaseline = 812.0;
constexpr double kFooterKeyX = 495.0;
constexpr double kFirstCellTop = 58.0;
constexpr double kCellHeight = 100.0;
constexpr double kColumnWidth = 72.0;
constexpr int kColumns = 7;
constexpr int kRowsPerPage = 7;

constexpr double kGlyphFontSize = 20.0;
constexpr double kGlyphBaseline = 20.0;
constexpr double kDurationFontSize = 14.0;
constexpr double kDurationBaseline = 34.0;
constexpr double kHoleFirstCenter = 46.0;
constexpr double kHoleSpacing = 6.4;
constexpr double kHoleRadius = 3.1;
constexpr double kHoleColumnOffset = 26.0;
constexpr double kLabelFontSize = 9.0;
constexpr double kLabelBaseline = 48.0;
constexpr double kLabelColumnOffset = 4.0;

constexpr double kFooterFontSize = 9.0;
constexpr double kLineWidth = 0.6;

int cellsPerPage() {
    return kColumns * kRowsPerPage;
}

void drawHoles(PageCanvas &canvas, double centerX, double firstCenterY,
               const Fingering &fingering) {
    for (std::size_t index = 0; index < fingering.holes.size(); ++index) {
        const double y = firstCenterY + static_cast<double>(index) * kHoleSpacing;
        const bool covered = fingering.holes.at(index) == HoleState::Covered;
        canvas.drawHole(centerX, y, kHoleRadius, covered);
    }
}

void drawNote(PageCanvas &canvas, const FingerTable &table, const Note &note, double cellTop,
              double columnX) {
    const Fingering fingering = table.lookup(note);
    canvas.setFontSize(kGlyphFontSize);
    canvas.drawText(columnX, cellTop + kGlyphBaseline,
                    QString::fromStdString(note_format::asciiGlyph(note)));
    const std::string duration = note_format::durationLabel(note);
    if (!duration.empty()) {
        canvas.setFontSize(kDurationFontSize);
        canvas.drawText(columnX, cellTop + kDurationBaseline, QString::fromStdString(duration));
    }
    if (!fingering.playable) {
        canvas.setFontSize(kLabelFontSize);
        canvas.drawText(columnX + kLabelColumnOffset, cellTop + kLabelBaseline,
                        QStringLiteral("?"));
        return;
    }
    drawHoles(canvas, columnX + kHoleColumnOffset, cellTop + kHoleFirstCenter, fingering);
}

void drawFooter(PageCanvas &canvas, const QString &keyName, std::size_t pageNumber,
                std::size_t pageCount) {
    canvas.setFontSize(kFooterFontSize);
    canvas.drawText(kMargin, kFooterBaseline,
                    QStringLiteral("page %1/%2")
                            .arg(static_cast<int>(pageNumber))
                            .arg(static_cast<int>(pageCount)));
    canvas.drawText(kFooterKeyX, kFooterBaseline, keyName);
}

void drawPage(PageCanvas &canvas, const FingerTable &table, const Score &score, std::size_t start,
              const QString &keyName, std::size_t pageNumber, std::size_t pageCount) {
    canvas.addPage();
    for (int cell = 0; cell < cellsPerPage(); ++cell) {
        const std::size_t index = start + static_cast<std::size_t>(cell);
        if (index >= score.size()) {
            break;
        }
        const double columnX = kMargin + static_cast<double>(cell % kColumns) * kColumnWidth;
        const double cellTop = kFirstCellTop + static_cast<double>(cell / kColumns) * kCellHeight;
        drawNote(canvas, table, score[index], cellTop, columnX);
    }
    drawFooter(canvas, keyName, pageNumber, pageCount);
}

std::size_t pageCountFor(const Score &score) {
    const std::size_t total = score.empty() ? 1 : score.size();
    const std::size_t perPage = static_cast<std::size_t>(cellsPerPage());
    return (total + perPage - 1) / perPage;
}

}

PdfRenderer::PdfRenderer(CanvasKind kind)
    : m_kind(kind) {
}

bool PdfRenderer::render(const std::vector<Score> &scores, const QString &outputPath,
                         QString &error) {
    if (scores.empty()) {
        error = QStringLiteral("没有可渲染的内容：没有识别到任何简谱。");
        return false;
    }
    if (outputPath.trimmed().isEmpty()) {
        error = QStringLiteral("输出 PDF 路径为空。");
        return false;
    }
    const QString keyName = QString::fromStdString(m_fingerTable.keyName());
    const std::unique_ptr<PageCanvas> canvas = makePageCanvas(m_kind);
    if (!canvas->prepare(QStringList{keyName}, outputPath, QSizeF(kPageWidth, kPageHeight), error)) {
        return false;
    }
    canvas->setLineWidth(kLineWidth);
    std::size_t total = 0;
    for (const Score &score : scores) {
        total += pageCountFor(score);
    }
    std::size_t pageNumber = 0;
    for (const Score &score : scores) {
        const std::size_t count = pageCountFor(score);
        for (std::size_t index = 0; index < count; ++index) {
            ++pageNumber;
            drawPage(*canvas, m_fingerTable, score,
                     index * static_cast<std::size_t>(cellsPerPage()), keyName, pageNumber, total);
        }
    }
    return canvas->finish(error);
}

}
