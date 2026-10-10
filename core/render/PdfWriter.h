#ifndef DIZITABFORGE_CORE_RENDER_PDFWRITER_H
#define DIZITABFORGE_CORE_RENDER_PDFWRITER_H

#include <string>
#include <vector>

namespace dizi {

class PdfWriter {
public:
    PdfWriter(double pageWidth, double pageHeight);

    void addPage();

    void setFontSize(double size);

    void setTextColor(double red, double green, double blue);

    void drawText(double x, double y, const std::string &text);

    void drawCircle(double centerX, double centerY, double radius, bool filled);

    void setLineWidth(double width);

    bool save(const std::string &path, std::string &error) const;

private:
    struct PageContent {
        std::string text;
    };

    std::string buildContentStream(const PageContent &page) const;

    std::string buildPageObject(const PageContent &page, int contentNumber) const;

    std::string buildPageTreeObject() const;

    std::string buildCatalogObject() const;

    std::string buildFontObject() const;

    std::string buildCrossReference(const std::vector<std::size_t> &offsets) const;

    std::string assemble(const std::vector<std::string> &objects) const;

    double m_pageWidth;
    double m_pageHeight;
    double m_fontSize = 12.0;
    double m_textRed = 0.0;
    double m_textGreen = 0.0;
    double m_textBlue = 0.0;
    double m_lineWidth = 0.7;
    std::vector<PageContent> m_pages;
};

}

#endif
