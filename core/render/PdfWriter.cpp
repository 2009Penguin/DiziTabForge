#include "core/render/PdfWriter.h"

#include <cstdio>
#include <fstream>

namespace dizi {

namespace {

constexpr int kCatalogNumber = 1;
constexpr int kPageTreeNumber = 2;
constexpr int kFontNumber = 3;
constexpr int kNumberBase = 4;
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
    target += number(m_fontSize) + " Tf" + std::string(1, kNewLine);
    target += color + " rg" + std::string(1, kNewLine);
    target += number(x) + " " + number(y) + " Td" + std::string(1, kNewLine);
    target += "(" + escapeLiteral(text) + ") Tj" + std::string(1, kNewLine);
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

std::string PdfWriter::buildPageObject(const PageContent &page, int contentNumber) const {
    const std::string body = buildContentStream(page);
    const int contentObject = kNumberBase + contentNumber;
    const int pageObject = contentObject + 1;
    std::string object = std::to_string(contentObject) + " 0 obj" + kNewLine + "<<" + kNewLine +
                         "/Length " + std::to_string(body.size()) + kNewLine + ">>" + kNewLine +
                         "stream" + kNewLine + body + "endstream" + kNewLine + "endobj" + kNewLine;
    object += std::to_string(pageObject) + " 0 obj" + kNewLine + "<<" + kNewLine + "/Type /Page" +
              kNewLine + "/Parent " + std::to_string(kPageTreeNumber) + " 0 R" + kNewLine +
              "/MediaBox [0 0 " + number(m_pageWidth) + " " + number(m_pageHeight) + "]" + kNewLine +
              "/Resources << /Font << /F1 " + std::to_string(kFontNumber) + " 0 R >> >>" + kNewLine +
              "/Contents " + std::to_string(contentObject) + " 0 R" + kNewLine + ">>" + kNewLine +
              "endobj" + kNewLine;
    return object;
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

std::string PdfWriter::buildCrossReference(const std::vector<std::size_t> &offsets) const {
    std::string xref = "xref" + std::string(1, kNewLine) + "0 " +
                       std::to_string(offsets.size()) + kNewLine;
    xref += "0000000000 65535 f " + std::string(1, kNewLine);
    for (std::size_t index = 1; index < offsets.size(); ++index) {
        char entry[32] = {};
        std::snprintf(entry, sizeof(entry), "%010llu 00000 n ",
                      static_cast<unsigned long long>(offsets[index]));
        xref += entry;
        xref += kNewLine;
    }
    return xref;
}

std::string PdfWriter::assemble(const std::vector<std::string> &objects) const {
    std::string document = "%PDF-1.4" + std::string(1, kNewLine);
    std::vector<std::size_t> offsets;
    offsets.reserve(objects.size() + 1);
    offsets.push_back(0);
    for (const std::string &object : objects) {
        offsets.push_back(document.size());
        document += object;
    }
    const std::size_t xrefOffset = document.size();
    document += buildCrossReference(offsets);
    document += "trailer" + std::string(1, kNewLine) + "<< /Size " +
                std::to_string(offsets.size()) + " /Root " + std::to_string(kCatalogNumber) +
                " 0 R >>" + kNewLine;
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
    std::vector<std::string> objects;
    objects.reserve(m_pages.size() * 2 + 3);
    objects.push_back(buildCatalogObject());
    objects.push_back(buildPageTreeObject());
    objects.push_back(buildFontObject());
    for (std::size_t index = 0; index < m_pages.size(); ++index) {
        objects.push_back(buildPageObject(m_pages[index], static_cast<int>(index) * 2));
    }
    const std::string document = assemble(objects);
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
    error.clear();
    return true;
}

}
