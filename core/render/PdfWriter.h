#ifndef DIZITABFORGE_CORE_RENDER_PDFWRITER_H
#define DIZITABFORGE_CORE_RENDER_PDFWRITER_H

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core/render/EmbeddedFont.h"

namespace dizi {

class PdfWriter {
public:
    PdfWriter(double pageWidth, double pageHeight);

    void setEmbeddedFont(const EmbeddedFont &font);

    bool hasEmbeddedFont() const;

    bool canRenderCjk(char32_t codepoint) const;

    void addPage();

    void setFontSize(double size);

    void setTextColor(double red, double green, double blue);

    void drawText(double x, double y, const std::string &text);

    void drawCjkText(double x, double y, const std::string &text);

    void drawCircle(double centerX, double centerY, double radius, bool filled);

    void setLineWidth(double width);

    bool save(const std::string &path, std::string &error) const;

private:
    struct PageContent {
        std::string text;
    };

    std::string buildContentStream(const PageContent &page) const;

    std::string buildContentObject(const PageContent &page, int index) const;

    std::string buildPageObject(int index) const;

    std::string buildPageTreeObject() const;

    std::string buildCatalogObject() const;

    std::string buildFontObject() const;

    std::string buildCjkFontObject() const;

    std::string buildCjkDescriptorObject() const;

    std::string buildCjkProgramObject() const;

    std::string buildCjkToUnicodeObject() const;

    std::string buildCidFontObject() const;

    void appendPageObjects(std::vector<std::pair<int, std::string>> &objects) const;

    void appendCjkObjects(std::vector<std::pair<int, std::string>> &objects) const;

    std::string buildDocument() const;

    bool writeDocument(const std::string &path, const std::string &document,
                       std::string &error) const;

    std::string buildCjkWidths() const;

    std::string buildToUnicode() const;

    std::string buildFontResources() const;

    std::string buildCrossReference(const std::map<int, std::size_t> &offsets) const;

    std::string assemble(const std::vector<std::pair<int, std::string>> &objects) const;

    std::string encodeCjk(const std::string &text) const;

    double m_pageWidth;
    double m_pageHeight;
    double m_fontSize = 12.0;
    double m_textRed = 0.0;
    double m_textGreen = 0.0;
    double m_textBlue = 0.0;
    double m_lineWidth = 0.7;
    std::vector<PageContent> m_pages;
    EmbeddedFont m_embedded;
};

}

#endif
