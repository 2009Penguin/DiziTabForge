#include "core/render/PdfWriter.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>

#include "core/render/TrueTypeFace.h"

namespace dizi {

namespace {

constexpr int kCatalogNumber = 1;
constexpr int kPageTreeNumber = 2;
constexpr int kFontNumber = 3;
constexpr int kNumberBase = 4;
constexpr int kCjkBase = 1000;
constexpr char kAsciiFontName[] = "/F1 ";
constexpr char kCjkFontName[] = "/F2 ";
constexpr double kCircleKappa = 0.5522847498;
constexpr char kNewLine = '\n';

std::string escapeLiteral(const std::string &text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (char symbol : text) {
        if (symbol == '(' || symbol == ')' || symbol == '\\') {
            escaped.push_back('\\');
        }
        escaped.push_back(symbol);
    }
    return escaped;
}

std::string number(double value) {
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%.4f", value);
    std::string text(buffer);
    while (text.size() > 1 && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text.empty() ? std::string("0") : text;
}

}

PdfWriter::PdfWriter(double pageWidth, double pageHeight)
    : m_pageWidth(pageWidth),
      m_pageHeight(pageHeight) {
}

void PdfWriter::setEmbeddedFont(const EmbeddedFont &font) {
    m_embedded = font;
}

bool PdfWriter::hasEmbeddedFont() const {
    return m_embedded.usable;
}

bool PdfWriter::canRenderCjk(char32_t codepoint) const {
    const auto found = m_embedded.cidByCodepoint.find(codepoint);
    return found != m_embedded.cidByCodepoint.end() && found->second != 0;
}

void PdfWriter::addPage() {
    m_pages.push_back(PageContent());
}

void PdfWriter::setFontSize(double size) {
    m_fontSize = size;
}

void PdfWriter::setTextColor(double red, double green, double blue) {
    m_textRed = red;
    m_textGreen = green;
    m_textBlue = blue;
}

void PdfWriter::setLineWidth(double width) {
    m_lineWidth = width;
}

void PdfWriter::drawText(double x, double y, const std::string &text) {
    if (m_pages.empty() || text.empty()) {
        return;
    }
    std::string &target = m_pages.back().text;
    const std::string color = number(m_textRed) + " " + number(m_textGreen) + " " +
                              number(m_textBlue);
    target += "BT" + std::string(1, kNewLine);
    target += std::string(kAsciiFontName) + number(m_fontSize) + " Tf" + std::string(1, kNewLine);
    target += color + " rg" + std::string(1, kNewLine);
    target += number(x) + " " + number(y) + " Td" + std::string(1, kNewLine);
    target += "(" + escapeLiteral(text) + ") Tj" + std::string(1, kNewLine);
    target += "ET" + std::string(1, kNewLine);
}

std::string PdfWriter::encodeCjk(const std::string &text) const {
    std::string encoded;
    char buffer[8] = {};
    for (char32_t codepoint : TrueTypeFace::decodeUtf8(text)) {
        const auto found = m_embedded.cidByCodepoint.find(codepoint);
        const unsigned glyph = found == m_embedded.cidByCodepoint.end() ? 0U : found->second;
        std::snprintf(buffer, sizeof(buffer), "%04X", glyph);
        encoded += buffer;
    }
    return encoded;
}

void PdfWriter::drawCjkText(double x, double y, const std::string &text) {
    if (m_pages.empty() || text.empty() || !m_embedded.usable) {
        return;
    }
    const std::string encoded = encodeCjk(text);
    if (encoded.empty()) {
        return;
    }
    std::string &target = m_pages.back().text;
    const std::string color = number(m_textRed) + " " + number(m_textGreen) + " " +
                              number(m_textBlue);
    target += "BT" + std::string(1, kNewLine);
    target += std::string(kCjkFontName) + number(m_fontSize) + " Tf" + std::string(1, kNewLine);
    target += color + " rg" + std::string(1, kNewLine);
    target += number(x) + " " + number(y) + " Td" + std::string(1, kNewLine);
    target += "<" + encoded + "> Tj" + std::string(1, kNewLine);
    target += "ET" + std::string(1, kNewLine);
}

void PdfWriter::drawCircle(double centerX, double centerY, double radius, bool filled) {
    if (m_pages.empty() || radius <= 0.0) {
        return;
    }
    const double delta = radius * kCircleKappa;
    const double left = centerX - radius;
    const double right = centerX + radius;
    const double bottom = centerY - radius;
    const double top = centerY + radius;
    const std::string color = number(m_textRed) + " " + number(m_textGreen) + " " +
                              number(m_textBlue);
    std::string &target = m_pages.back().text;
    target += color + " RG" + std::string(1, kNewLine);
    target += number(m_lineWidth) + " w" + std::string(1, kNewLine);
    target += number(left) + " " + number(centerY) + " m" + std::string(1, kNewLine);
    target += number(left) + " " + number(centerY + delta) + " " + number(centerX - delta) + " " +
              number(top) + " " + number(centerX) + " " + number(top) + " c" +
              std::string(1, kNewLine);
    target += number(centerX + delta) + " " + number(top) + " " + number(right) + " " +
              number(centerY + delta) + " " + number(right) + " " + number(centerY) + " c" +
              std::string(1, kNewLine);
    target += number(right) + " " + number(centerY - delta) + " " + number(centerX + delta) + " " +
              number(bottom) + " " + number(centerX) + " " + number(bottom) + " c" +
              std::string(1, kNewLine);
    target += number(centerX - delta) + " " + number(bottom) + " " + number(left) + " " +
              number(centerY - delta) + " " + number(left) + " " + number(centerY) + " c" +
              std::string(1, kNewLine);
    target += filled ? "f" : "S";
    target += std::string(1, kNewLine);
}

std::string PdfWriter::buildContentStream(const PageContent &page) const {
    return page.text;
}

std::string PdfWriter::buildCatalogObject() const {
    return std::to_string(kCatalogNumber) + " 0 obj" + kNewLine + "<<" + kNewLine +
           "/Type /Catalog" + kNewLine + "/Pages " + std::to_string(kPageTreeNumber) + " 0 R" +
           kNewLine + ">>" + kNewLine + "endobj" + kNewLine;
}

std::string PdfWriter::buildFontObject() const {
    return std::to_string(kFontNumber) + " 0 obj" + kNewLine + "<<" + kNewLine + "/Type /Font" +
           kNewLine + "/Subtype /Type1" + kNewLine + "/BaseFont /Helvetica" + kNewLine +
           "/Encoding /WinAnsiEncoding" + kNewLine + ">>" + kNewLine + "endobj" + kNewLine;
}

std::string PdfWriter::buildContentObject(const PageContent &page, int index) const {
    const std::string body = buildContentStream(page);
    const int contentObject = kNumberBase + index * 2;
    return std::to_string(contentObject) + " 0 obj" + kNewLine + "<<" + kNewLine + "/Length " +
           std::to_string(body.size()) + kNewLine + ">>" + kNewLine + "stream" + kNewLine + body +
           "endstream" + kNewLine + "endobj" + kNewLine;
}

std::string PdfWriter::buildFontResources() const {
    std::string resources = "/F1 " + std::to_string(kFontNumber) + " 0 R";
    if (m_embedded.usable) {
        resources += " /F2 " + std::to_string(kCjkBase) + " 0 R";
    }
    return resources;
}

std::string PdfWriter::buildPageObject(int index) const {
    const int contentObject = kNumberBase + index * 2;
    const int pageObject = contentObject + 1;
    return std::to_string(pageObject) + " 0 obj" + kNewLine + "<<" + kNewLine + "/Type /Page" +
           kNewLine + "/Parent " + std::to_string(kPageTreeNumber) + " 0 R" + kNewLine +
           "/MediaBox [0 0 " + number(m_pageWidth) + " " + number(m_pageHeight) + "]" + kNewLine +
           "/Resources << /Font << " + buildFontResources() + " >> >>" + kNewLine +
           "/Contents " + std::to_string(contentObject) + " 0 R" + kNewLine + ">>" + kNewLine +
           "endobj" + kNewLine;
}

std::string PdfWriter::buildCjkWidths() const {
    const double scale = 1000.0 / static_cast<double>(m_embedded.unitsPerEm);
    std::string widths;
    for (std::size_t cid = 0; cid < m_embedded.advanceByCid.size(); ++cid) {
        if (cid == 0) {
            continue;
        }
        widths += " " + std::to_string(cid) + " [" +
                  number(static_cast<double>(m_embedded.advanceByCid[cid]) * scale) + "]";
    }
    return widths;
}

std::string PdfWriter::buildToUnicode() const {
    std::string cmap = "/CIDInit /ProcSet findresource begin\n12 dict begin\nbegincmap\n"
                       "/CIDSystemInfo << /Registry (Adobe) /Ordering (UCS) /Supplement 0 >> def\n"
                       "/CMapName /Adobe-Identity-UCS def\n/CMapType 2 def\n"
                       "1 begincodespacerange\n<0000> <FFFF>\nendcodespacerange\n";
    const std::size_t count = m_embedded.unicodeByCid.size();
    for (std::size_t start = 0; start < count; start += 100) {
        const std::size_t end = std::min(start + 100, count);
        cmap += std::to_string(end - start) + " beginbfchar\n";
        for (std::size_t index = start; index < end; ++index) {
            char line[48] = {};
            std::snprintf(line, sizeof(line), "<%04X> <%04X>\n",
                          m_embedded.unicodeByCid[index].second,
                          static_cast<unsigned>(m_embedded.unicodeByCid[index].first));
            cmap += line;
        }
        cmap += "endbfchar\n";
    }
    cmap += "endcmap\nCMapName currentdict /CMap defineresource pop\nend\nend\n";
    return cmap;
}

std::string PdfWriter::buildCjkFontObject() const {
    return std::to_string(kCjkBase) + " 0 obj" + kNewLine + "<<" + kNewLine + "/Type /Font" +
           kNewLine + "/Subtype /Type0" + kNewLine + "/BaseFont /" + m_embedded.postScriptName +
           kNewLine + "/Encoding /Identity-H" + kNewLine + "/DescendantFonts [" +
           std::to_string(kCjkBase + 4) + " 0 R]" + kNewLine + "/ToUnicode " +
           std::to_string(kCjkBase + 3) + " 0 R" + kNewLine + ">>" + kNewLine + "endobj" + kNewLine;
}

std::string PdfWriter::buildCjkDescriptorObject() const {
    const double scale = 1000.0 / static_cast<double>(m_embedded.unitsPerEm);
    const double ascent = static_cast<double>(m_embedded.ascent) * scale;
    const double descent = static_cast<double>(m_embedded.descent) * scale;
    return std::to_string(kCjkBase + 1) + " 0 obj" + kNewLine + "<<" + kNewLine +
           "/Type /FontDescriptor" + kNewLine + "/FontName /" + m_embedded.postScriptName +
           kNewLine + "/Flags 4" + kNewLine + "/FontBBox [0 " + number(descent) + " 1000 " +
           number(ascent) + "]" + kNewLine + "/ItalicAngle 0" + kNewLine + "/Ascent " +
           number(ascent) + kNewLine + "/Descent " + number(descent) + kNewLine + "/CapHeight " +
           number(ascent) + kNewLine + "/StemV 80" + kNewLine + "/FontFile2 " +
           std::to_string(kCjkBase + 2) + " 0 R" + kNewLine + ">>" + kNewLine + "endobj" + kNewLine;
}

std::string PdfWriter::buildCjkProgramObject() const {
    return std::to_string(kCjkBase + 2) + " 0 obj" + kNewLine + "<<" + kNewLine + "/Length " +
           std::to_string(m_embedded.program.size()) + kNewLine + "/Length1 " +
           std::to_string(m_embedded.program.size()) + kNewLine + ">>" + kNewLine + "stream" +
           kNewLine + m_embedded.program + "endstream" + kNewLine + "endobj" + kNewLine;
}

std::string PdfWriter::buildCjkToUnicodeObject() const {
    const std::string cmap = buildToUnicode();
    return std::to_string(kCjkBase + 3) + " 0 obj" + kNewLine + "<<" + kNewLine + "/Length " +
           std::to_string(cmap.size()) + kNewLine + ">>" + kNewLine + "stream" + kNewLine + cmap +
           "endstream" + kNewLine + "endobj" + kNewLine;
}

std::string PdfWriter::buildCidFontObject() const {
    return std::to_string(kCjkBase + 4) + " 0 obj" + kNewLine + "<<" + kNewLine + "/Type /Font" +
           kNewLine + "/Subtype /CIDFontType2" + kNewLine + "/BaseFont /" +
           m_embedded.postScriptName + kNewLine + "/CIDSystemInfo << /Registry (Adobe) " +
           "/Ordering (Identity) /Supplement 0 >>" + kNewLine + "/FontDescriptor " +
           std::to_string(kCjkBase + 1) + " 0 R" + kNewLine + "/DW 1000" + kNewLine + "/W [" +
           buildCjkWidths() + " ]" + kNewLine + "/CIDToGIDMap /Identity" + kNewLine + ">>" +
           kNewLine + "endobj" + kNewLine;
}

std::string PdfWriter::buildPageTreeObject() const {
    std::string kids;
    for (std::size_t index = 0; index < m_pages.size(); ++index) {
        if (index > 0) {
            kids += " ";
        }
        kids += std::to_string(kNumberBase + static_cast<int>(index) * 2 + 1) + " 0 R";
    }
    return std::to_string(kPageTreeNumber) + " 0 obj" + kNewLine + "<<" + kNewLine +
           "/Type /Pages" + kNewLine + "/Kids [" + kids + "]" + kNewLine + "/Count " +
           std::to_string(m_pages.size()) + kNewLine + ">>" + kNewLine + "endobj" + kNewLine;
}

std::string PdfWriter::buildCrossReference(const std::map<int, std::size_t> &offsets) const {
    const int highest = offsets.empty() ? 0 : offsets.rbegin()->first;
    const std::size_t count = static_cast<std::size_t>(highest) + 1;
    std::string xref = "xref" + std::string(1, kNewLine) + "0 " + std::to_string(count) +
                       std::string(1, kNewLine);
    xref += "0000000000 65535 f " + std::string(1, kNewLine);
    for (int index = 1; index <= highest; ++index) {
        char entry[32] = {};
        const auto found = offsets.find(index);
        if (found == offsets.end()) {
            std::snprintf(entry, sizeof(entry), "0000000000 00000 f ");
        } else {
            std::snprintf(entry, sizeof(entry), "%010llu 00000 n ",
                          static_cast<unsigned long long>(found->second));
        }
        xref += entry;
        xref += kNewLine;
    }
    xref += "trailer" + std::string(1, kNewLine) + "<< /Size " + std::to_string(count) +
            " /Root " + std::to_string(kCatalogNumber) + " 0 R >>" + std::string(1, kNewLine);
    return xref;
}

std::string PdfWriter::assemble(const std::vector<std::pair<int, std::string>> &objects) const {
    std::string document = "%PDF-1.4" + std::string(1, kNewLine);
    std::map<int, std::size_t> offsets;
    for (const auto &object : objects) {
        offsets[object.first] = document.size();
        document += object.second;
    }
    const std::size_t xrefOffset = document.size();
    document += buildCrossReference(offsets);
    document += "startxref" + std::string(1, kNewLine) + std::to_string(xrefOffset) +
                std::string(1, kNewLine);
    document += "%%EOF" + std::string(1, kNewLine);
    return document;
}

bool PdfWriter::save(const std::string &path, std::string &error) const {
    if (m_pages.empty()) {
        error = "the document has no page";
        return false;
    }
    if (kNumberBase + static_cast<int>(m_pages.size()) * 2 + 1 >= kCjkBase) {
        error = "too many pages for the reserved object numbers";
        return false;
    }
    const std::string document = buildDocument();
    if (!writeDocument(path, document, error)) {
        return false;
    }
    error.clear();
    return true;
}

void PdfWriter::appendPageObjects(std::vector<std::pair<int, std::string>> &objects) const {
    for (std::size_t index = 0; index < m_pages.size(); ++index) {
        const int number = static_cast<int>(index);
        objects.emplace_back(kNumberBase + number * 2, buildContentObject(m_pages[index], number));
        objects.emplace_back(kNumberBase + number * 2 + 1, buildPageObject(number));
    }
}

void PdfWriter::appendCjkObjects(std::vector<std::pair<int, std::string>> &objects) const {
    if (!m_embedded.usable) {
        return;
    }
    objects.emplace_back(kCjkBase, buildCjkFontObject());
    objects.emplace_back(kCjkBase + 1, buildCjkDescriptorObject());
    objects.emplace_back(kCjkBase + 2, buildCjkProgramObject());
    objects.emplace_back(kCjkBase + 3, buildCjkToUnicodeObject());
    objects.emplace_back(kCjkBase + 4, buildCidFontObject());
}

std::string PdfWriter::buildDocument() const {
    std::vector<std::pair<int, std::string>> objects;
    objects.reserve(m_pages.size() * 2 + 9);
    objects.emplace_back(kCatalogNumber, buildCatalogObject());
    objects.emplace_back(kPageTreeNumber, buildPageTreeObject());
    objects.emplace_back(kFontNumber, buildFontObject());
    appendPageObjects(objects);
    appendCjkObjects(objects);
    std::sort(objects.begin(), objects.end(),
              [](const auto &left, const auto &right) { return left.first < right.first; });
    return assemble(objects);
}

bool PdfWriter::writeDocument(const std::string &path, const std::string &document,
                              std::string &error) const {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        error = "cannot open " + path + " for writing";
        return false;
    }
    stream.write(document.data(), static_cast<std::streamsize>(document.size()));
    stream.close();
    if (!stream) {
        error = "failed while writing " + path;
        return false;
    }
    return true;
}

}
