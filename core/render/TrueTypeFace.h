#ifndef DIZITABFORGE_CORE_RENDER_TRUETYPEFACE_H
#define DIZITABFORGE_CORE_RENDER_TRUETYPEFACE_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace dizi {

struct FontMetrics {
    double ascent = 0.0;
    double descent = 0.0;
    int unitsPerEm = 1000;
    int glyphCount = 0;
};

struct GlyphSubset {
    std::vector<unsigned> glyphs;
    std::map<unsigned, unsigned> newIndex;
    std::map<char32_t, unsigned> cidByCodepoint;
    bool empty = true;
};

class TrueTypeFace {
public:
    bool load(const std::string &path);

    bool loadFromMemory(const std::vector<std::uint8_t> &data);

    bool isLoaded() const;

    const FontMetrics &metrics() const;

    unsigned glyphFor(char32_t codepoint) const;

    std::uint16_t advanceFor(unsigned glyph) const;

    std::vector<unsigned> componentGlyphs(unsigned glyph) const;

    void expandClosure(std::vector<unsigned> &glyphs) const;

    bool buildSubset(const std::vector<char32_t> &codepoints, GlyphSubset &subset,
                     std::string &fontFile, std::string &error) const;

    static std::vector<char32_t> decodeUtf8(const std::string &text);

private:
    struct TableRecord {
        std::uint32_t offset = 0;
        std::uint32_t length = 0;
    };

    struct Table {
        std::string tag;
        std::vector<std::uint8_t> data;
    };

    bool findTable(const char *tag, TableRecord &record) const;

    bool parseHead();

    bool parseHhea();

    bool parseMaxp();

    bool parseHmtx();

    bool parseCmap();

    bool parseLoca();

    unsigned glyphLength(unsigned glyph) const;

    const std::vector<std::uint8_t> &glyphData(unsigned glyph) const;

    std::vector<std::uint8_t> buildGlyf(const GlyphSubset &subset) const;

    std::vector<std::uint8_t> buildLoca(const GlyphSubset &subset,
                                        const std::vector<std::uint8_t> &glyf) const;

    std::vector<std::uint8_t> buildHead(std::size_t locaBytes) const;

    std::vector<std::uint8_t> buildMaxp(int glyphCount) const;

    std::vector<std::uint8_t> buildHmtx(const GlyphSubset &subset) const;

    std::vector<std::uint8_t> buildPost() const;

    std::vector<std::uint8_t> buildOs2() const;

    std::vector<std::uint8_t> buildName() const;

    std::vector<std::uint8_t> buildEmptyTable() const;

    void patchCompositeGlyph(std::vector<std::uint8_t> &data,
                             const std::map<unsigned, unsigned> &newIndex) const;

    void serializeFont(const std::vector<Table> &tables, std::string &target) const;

    unsigned newIndexFor(const GlyphSubset &subset, unsigned glyph) const;

    std::vector<std::uint8_t> m_data;
    std::map<std::string, TableRecord> m_tables;
    std::vector<std::uint16_t> m_advances;
    std::vector<std::uint32_t> m_cmapOffsets;
    std::vector<std::uint32_t> m_loca;
    FontMetrics m_metrics;
    TableRecord m_head;
    TableRecord m_hhea;
    TableRecord m_maxp;
    TableRecord m_glyf;
    TableRecord m_locaTable;
    bool m_longLoca = false;
};

}

#endif
