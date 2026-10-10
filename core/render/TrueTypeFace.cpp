#include "core/render/TrueTypeFace.h"

#include <algorithm>
#include <fstream>
#include <set>

namespace dizi {

namespace {

constexpr unsigned kMissingGlyph = 0;

std::uint16_t readU16(const std::vector<std::uint8_t> &data, std::size_t offset) {
    return static_cast<std::uint16_t>((data.at(offset) << 8) | data.at(offset + 1));
}

std::int16_t readI16(const std::vector<std::uint8_t> &data, std::size_t offset) {
    return static_cast<std::int16_t>(readU16(data, offset));
}

std::int8_t readI8(const std::vector<std::uint8_t> &data, std::size_t offset) {
    return static_cast<std::int8_t>(data.at(offset));
}

std::uint32_t readU32(const std::vector<std::uint8_t> &data, std::size_t offset) {
    return (static_cast<std::uint32_t>(data.at(offset)) << 24) |
           (static_cast<std::uint32_t>(data.at(offset + 1)) << 16) |
           (static_cast<std::uint32_t>(data.at(offset + 2)) << 8) |
           static_cast<std::uint32_t>(data.at(offset + 3));
}

void appendU16(std::vector<std::uint8_t> &out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value & 0xFF));
}

void appendU32(std::vector<std::uint8_t> &out, std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 24));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(value & 0xFF));
}

std::uint32_t tableChecksum(const std::vector<std::uint8_t> &table) {
    std::uint32_t sum = 0;
    for (std::size_t index = 0; index < table.size(); index += 4) {
        std::uint32_t word = 0;
        for (std::size_t byte = 0; byte < 4; ++byte) {
            word <<= 8;
            if (index + byte < table.size()) {
                word |= table[index + byte];
            }
        }
        sum += word;
    }
    return sum;
}

bool isComposite(const std::vector<std::uint8_t> &glyph) {
    return glyph.size() >= 10 && readI16(glyph, 0) < 0;
}

}

bool TrueTypeFace::load(const std::string &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }
    const std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(stream),
                                         std::istreambuf_iterator<char>()};
    return loadFromMemory(data);
}

bool TrueTypeFace::loadFromMemory(const std::vector<std::uint8_t> &data) {
    m_data = data;
    m_tables.clear();
    m_advances.clear();
    m_cmapOffsets.clear();
    m_loca.clear();
    m_metrics = FontMetrics();
    if (m_data.size() < 12) {
        return false;
    }
    const std::size_t tableCount = readU16(m_data, 4);
    for (std::size_t index = 0; index < tableCount; ++index) {
        const std::size_t record = 12 + index * 16;
        if (record + 16 > m_data.size()) {
            return false;
        }
        char tag[5] = {};
        for (int byte = 0; byte < 4; ++byte) {
            tag[byte] = static_cast<char>(m_data[record + static_cast<std::size_t>(byte)]);
        }
        TableRecord entry;
        entry.offset = readU32(m_data, record + 8);
        entry.length = readU32(m_data, record + 12);
        m_tables[tag] = entry;
    }
    if (m_tables.find("glyf") == m_tables.end() || m_tables.find("loca") == m_tables.end()) {
        return false;
    }
    return parseHead() && parseHhea() && parseMaxp() && parseHmtx() && parseCmap() && parseLoca();
}

bool TrueTypeFace::findTable(const char *tag, TableRecord &record) const {
    const auto found = m_tables.find(tag);
    if (found == m_tables.end()) {
        return false;
    }
    if (found->second.offset + found->second.length > m_data.size()) {
        return false;
    }
    record = found->second;
    return true;
}

bool TrueTypeFace::parseHead() {
    if (!findTable("head", m_head)) {
        return false;
    }
    m_metrics.unitsPerEm = readU16(m_data, m_head.offset + 18);
    if (m_metrics.unitsPerEm <= 0) {
        m_metrics.unitsPerEm = 1000;
    }
    m_longLoca = readI16(m_data, m_head.offset + 50) != 0;
    return true;
}

bool TrueTypeFace::parseHhea() {
    if (!findTable("hhea", m_hhea)) {
        return false;
    }
    m_metrics.ascent = static_cast<double>(readI16(m_data, m_hhea.offset + 4));
    m_metrics.descent = static_cast<double>(readI16(m_data, m_hhea.offset + 6));
    return true;
}

bool TrueTypeFace::parseMaxp() {
    if (!findTable("maxp", m_maxp)) {
        return false;
    }
    m_metrics.glyphCount = readU16(m_data, m_maxp.offset + 4);
    return m_metrics.glyphCount > 0;
}

bool TrueTypeFace::parseHmtx() {
    const std::size_t numberOfMetrics = readU16(m_data, m_hhea.offset + 34);
    m_advances.assign(static_cast<std::size_t>(m_metrics.glyphCount), 0);
    std::uint16_t last = 0;
    for (std::size_t index = 0; index < m_advances.size(); ++index) {
        if (index < numberOfMetrics) {
            last = readU16(m_data, m_hhea.offset + 36 + index * 4);
        }
        m_advances[index] = last;
    }
    return true;
}

bool TrueTypeFace::parseLoca() {
    if (!findTable("loca", m_locaTable)) {
        return false;
    }
    const std::size_t count = static_cast<std::size_t>(m_metrics.glyphCount) + 1;
    m_loca.assign(count, 0);
    for (std::size_t index = 0; index < count; ++index) {
        m_loca[index] = m_longLoca ? readU32(m_data, m_locaTable.offset + index * 4)
                                   : readU16(m_data, m_locaTable.offset + index * 2) * 2U;
    }
    return true;
}

bool TrueTypeFace::parseCmap() {
    TableRecord cmap;
    if (!findTable("cmap", cmap)) {
        return false;
    }
    const std::size_t count = readU16(m_data, cmap.offset + 2);
    for (std::size_t index = 0; index < count; ++index) {
        const std::size_t record = cmap.offset + 4 + index * 8;
        const std::uint16_t platform = readU16(m_data, record);
        const std::uint16_t encoding = readU16(m_data, record + 2);
        const std::uint32_t subtable = cmap.offset + readU32(m_data, record + 4);
        const bool unicode = platform == 0 || (platform == 3 && (encoding == 1 || encoding == 10));
        if (unicode && subtable + 4 <= m_data.size()) {
            m_cmapOffsets.push_back(subtable);
        }
    }
    return !m_cmapOffsets.empty();
}

unsigned TrueTypeFace::glyphFor(char32_t codepoint) const {
    const std::uint32_t code = static_cast<std::uint32_t>(codepoint);
    for (const std::uint32_t subtable : m_cmapOffsets) {
        const std::uint16_t format = readU16(m_data, subtable);
        if (format == 4) {
            const std::size_t segCount = readU16(m_data, subtable + 6) / 2;
            const std::size_t ends = subtable + 14;
            const std::size_t starts = ends + segCount * 2 + 2;
            const std::size_t deltas = starts + segCount * 2;
            const std::size_t ranges = deltas + segCount * 2;
            for (std::size_t segment = 0; segment < segCount; ++segment) {
                const std::uint16_t end = readU16(m_data, ends + segment * 2);
                if (code > end) {
                    continue;
                }
                const std::uint16_t start = readU16(m_data, starts + segment * 2);
                if (code < start) {
                    break;
                }
                const std::uint16_t delta = readU16(m_data, deltas + segment * 2);
                const std::uint16_t rangeOffset = readU16(m_data, ranges + segment * 2);
                if (rangeOffset == 0) {
                    return (code + delta) & 0xFFFFU;
                }
                const std::size_t address =
                        ranges + segment * 2 + rangeOffset + (code - start) * 2;
                if (address + 2 > m_data.size()) {
                    return kMissingGlyph;
                }
                const std::uint16_t glyph = readU16(m_data, address);
                return glyph == 0 ? kMissingGlyph : (glyph + delta) & 0xFFFFU;
            }
        } else if (format == 12) {
            const std::uint32_t groups = readU32(m_data, subtable + 12);
            for (std::uint32_t group = 0; group < groups; ++group) {
                const std::size_t base = subtable + 16 + group * 12;
                if (base + 12 > m_data.size()) {
                    break;
                }
                const std::uint32_t start = readU32(m_data, base);
                const std::uint32_t end = readU32(m_data, base + 4);
                if (code >= start && code <= end) {
                    return readU32(m_data, base + 8) + (code - start);
                }
            }
        }
    }
    return kMissingGlyph;
}

std::uint16_t TrueTypeFace::advanceFor(unsigned glyph) const {
    return glyph < m_advances.size() ? m_advances[glyph] : 0;
}

unsigned TrueTypeFace::glyphLength(unsigned glyph) const {
    return glyph + 1 < m_loca.size() ? m_loca[glyph + 1] - m_loca[glyph] : 0;
}

const std::vector<std::uint8_t> &TrueTypeFace::glyphData(unsigned glyph) const {
    static const std::vector<std::uint8_t> empty;
    if (glyph + 1 >= m_loca.size()) {
        return empty;
    }
    const std::size_t start = m_glyf.offset + m_loca[glyph];
    const std::size_t end = m_glyf.offset + m_loca[glyph + 1];
    if (end > m_data.size() || end <= start) {
        return empty;
    }
    static thread_local std::map<unsigned, std::vector<std::uint8_t>> cache;
    const auto found = cache.find(glyph);
    if (found != cache.end()) {
        return found->second;
    }
    return cache
            .emplace(glyph, std::vector<std::uint8_t>(m_data.begin() + start, m_data.begin() + end))
            .first->second;
}

std::vector<unsigned> TrueTypeFace::componentGlyphs(unsigned glyph) const {
    std::vector<unsigned> components;
    const std::vector<std::uint8_t> &data = glyphData(glyph);
    if (!isComposite(data)) {
        return components;
    }
    std::size_t offset = 10;
    while (offset + 4 <= data.size()) {
        const std::uint16_t flags = readU16(data, offset);
        components.push_back(readU16(data, offset + 2));
        offset += 4;
        offset += (flags & 0x0001) != 0 ? 4 : 2;
        if ((flags & 0x0008) != 0) {
            offset += 2;
        } else if ((flags & 0x0040) != 0) {
            offset += 4;
        } else if ((flags & 0x0080) != 0) {
            offset += 8;
        }
        if ((flags & 0x0020) == 0) {
            break;
        }
    }
    return components;
}

void TrueTypeFace::expandClosure(std::vector<unsigned> &glyphs) const {
    std::set<unsigned> seen(glyphs.begin(), glyphs.end());
    for (std::size_t index = 0; index < glyphs.size(); ++index) {
        for (unsigned component : componentGlyphs(glyphs[index])) {
            if (seen.insert(component).second) {
                glyphs.push_back(component);
            }
        }
    }
}

bool TrueTypeFace::isLoaded() const {
    return m_metrics.glyphCount > 0 && !m_tables.empty();
}

const FontMetrics &TrueTypeFace::metrics() const {
    return m_metrics;
}

std::vector<char32_t> TrueTypeFace::decodeUtf8(const std::string &text) {
    std::vector<char32_t> codepoints;
    std::size_t index = 0;
    while (index < text.size()) {
        const unsigned char lead = static_cast<unsigned char>(text[index]);
        char32_t code = lead;
        std::size_t extra = 0;
        if (lead >= 0xF0) {
            code = lead & 0x07;
            extra = 3;
        } else if (lead >= 0xE0) {
            code = lead & 0x0F;
            extra = 2;
        } else if (lead >= 0xC0) {
            code = lead & 0x1F;
            extra = 1;
        }
        ++index;
        for (std::size_t step = 0; step < extra && index < text.size(); ++step, ++index) {
            code = (code << 6) | (static_cast<unsigned char>(text[index]) & 0x3F);
        }
        codepoints.push_back(code);
    }
    return codepoints;
}

unsigned TrueTypeFace::newIndexFor(const GlyphSubset &subset, unsigned glyph) const {
    const auto found = subset.newIndex.find(glyph);
    return found == subset.newIndex.end() ? kMissingGlyph : found->second;
}

void TrueTypeFace::patchCompositeGlyph(std::vector<std::uint8_t> &data,
                                       const std::map<unsigned, unsigned> &newIndex) const {
    std::size_t offset = 10;
    while (offset + 4 <= data.size()) {
        const std::uint16_t flags = readU16(data, offset);
        const auto found = newIndex.find(readU16(data, offset + 2));
        const unsigned replacement = found == newIndex.end() ? kMissingGlyph : found->second;
        data[offset + 2] = static_cast<std::uint8_t>(replacement >> 8);
        data[offset + 3] = static_cast<std::uint8_t>(replacement & 0xFF);
        offset += 4;
        offset += (flags & 0x0001) != 0 ? 4 : 2;
        if ((flags & 0x0008) != 0) {
            offset += 2;
        } else if ((flags & 0x0040) != 0) {
            offset += 4;
        } else if ((flags & 0x0080) != 0) {
            offset += 8;
        }
        if ((flags & 0x0020) == 0) {
            break;
        }
    }
}

std::vector<std::uint8_t> TrueTypeFace::buildGlyf(const GlyphSubset &subset) const {
    std::vector<std::uint8_t> glyf;
    for (unsigned glyph : subset.glyphs) {
        const std::vector<std::uint8_t> &source = glyphData(glyph);
        if (source.size() < 10) {
            continue;
        }
        std::vector<std::uint8_t> outline = source;
        if (isComposite(outline)) {
            patchCompositeGlyph(outline, subset.newIndex);
        }
        glyf.insert(glyf.end(), outline.begin(), outline.end());
    }
    return glyf;
}

std::vector<std::uint8_t> TrueTypeFace::buildLoca(const GlyphSubset &subset,
                                                  const std::vector<std::uint8_t> &glyf) const {
    std::vector<std::uint8_t> loca;
    std::uint32_t offset = 0;
    for (unsigned glyph : subset.glyphs) {
        appendU32(loca, offset);
        const std::vector<std::uint8_t> &source = glyphData(glyph);
        if (source.size() >= 10) {
            offset += static_cast<std::uint32_t>(source.size());
        }
    }
    appendU32(loca, static_cast<std::uint32_t>(glyf.size()));
    return loca;
}

std::vector<std::uint8_t> TrueTypeFace::buildHead(std::size_t locaBytes) const {
    (void) locaBytes;
    std::vector<std::uint8_t> head(m_data.begin() + m_head.offset,
                                   m_data.begin() + m_head.offset + 54);
    head[50] = 0;
    head[51] = 1;
    head[52] = 0;
    head[53] = 0;
    head[8] = 0;
    head[9] = 0;
    return head;
}

std::vector<std::uint8_t> TrueTypeFace::buildMaxp(int glyphCount) const {
    std::vector<std::uint8_t> maxp;
    const std::size_t size = std::min<std::size_t>(m_maxp.length, 32);
    for (std::size_t index = 0; index < size; ++index) {
        maxp.push_back(m_data[m_maxp.offset + index]);
    }
    if (maxp.size() >= 6) {
        maxp[4] = static_cast<std::uint8_t>(glyphCount >> 8);
        maxp[5] = static_cast<std::uint8_t>(glyphCount & 0xFF);
    }
    return maxp;
}

std::vector<std::uint8_t> TrueTypeFace::buildHmtx(const GlyphSubset &subset) const {
    std::vector<std::uint8_t> hmtx;
    for (unsigned glyph : subset.glyphs) {
        appendU16(hmtx, advanceFor(glyph));
        appendU16(hmtx, 0);
    }
    return hmtx;
}

std::vector<std::uint8_t> TrueTypeFace::buildPost() const {
    std::vector<std::uint8_t> post;
    appendU32(post, 0x00030000U);
    appendU32(post, 0);
    appendU16(post, 0);
    appendU16(post, 0);
    appendU32(post, 0);
    appendU32(post, 0);
    appendU32(post, 0);
    appendU32(post, 0);
    appendU32(post, 0);
    return post;
}

std::vector<std::uint8_t> TrueTypeFace::buildOs2() const {
    std::vector<std::uint8_t> os2;
    appendU16(os2, 4);
    appendU16(os2, 400);
    appendU16(os2, 5);
    appendU16(os2, 0);
    appendU16(os2, 0);
    appendU16(os2, 0);
    for (int index = 0; index < 10; ++index) {
        appendU32(os2, 0);
    }
    for (int index = 0; index < 4; ++index) {
        for (int byte = 0; byte < 4; ++byte) {
            os2.push_back(byte == 0 && index == 0 ? static_cast<std::uint8_t>('D') : 0);
        }
    }
    appendU16(os2, static_cast<std::uint16_t>(m_metrics.ascent));
    appendU16(os2, static_cast<std::uint16_t>(m_metrics.descent));
    appendU16(os2, 0);
    appendU16(os2, static_cast<std::uint16_t>(m_metrics.ascent));
    appendU16(os2, static_cast<std::uint16_t>(m_metrics.descent));
    appendU32(os2, 0);
    appendU32(os2, 0);
    appendU16(os2, 0);
    return os2;
}

std::vector<std::uint8_t> TrueTypeFace::buildName() const {
    const std::string family = "DiziTabForgeCJK";
    const std::uint16_t nameIds[] = {1, 2, 4, 6};
    std::vector<std::uint8_t> storage;
    std::vector<std::uint8_t> records;
    for (int index = 0; index < 4; ++index) {
        const std::string value = index == 2 ? std::string("DiziTabForge CJK") : family;
        appendU16(records, 3);
        appendU16(records, 1);
        appendU16(records, 0x0409);
        appendU16(records, nameIds[index]);
        appendU16(records, static_cast<std::uint16_t>(value.size() * 2));
        appendU16(records, static_cast<std::uint16_t>(storage.size()));
        for (unsigned char symbol : value) {
            storage.push_back(0);
            storage.push_back(symbol);
        }
    }
    std::vector<std::uint8_t> name;
    appendU16(name, 0);
    appendU16(name, 4);
    appendU16(name, static_cast<std::uint16_t>(6 + records.size()));
    name.insert(name.end(), records.begin(), records.end());
    name.insert(name.end(), storage.begin(), storage.end());
    return name;
}

std::vector<std::uint8_t> TrueTypeFace::buildEmptyTable() const {
    return std::vector<std::uint8_t>();
}

void TrueTypeFace::serializeFont(const std::vector<Table> &tables, std::string &target) const {
    const std::size_t count = tables.size();
    std::size_t entrySelector = 0;
    std::size_t searchRange = 1;
    while (searchRange * 2 <= count) {
        searchRange *= 2;
        ++entrySelector;
    }
    searchRange *= 16;
    const std::size_t rangeShift = count * 16 - searchRange;
    std::vector<std::uint8_t> font;
    appendU32(font, 0x00010000U);
    appendU16(font, static_cast<std::uint16_t>(count));
    appendU16(font, static_cast<std::uint16_t>(searchRange));
    appendU16(font, static_cast<std::uint16_t>(entrySelector));
    appendU16(font, static_cast<std::uint16_t>(rangeShift));
    std::size_t offset = 12 + count * 16;
    for (const Table &table : tables) {
        for (char symbol : table.tag) {
            font.push_back(static_cast<std::uint8_t>(symbol));
        }
        appendU32(font, tableChecksum(table.data));
        appendU32(font, static_cast<std::uint32_t>(offset));
        appendU32(font, static_cast<std::uint32_t>(table.data.size()));
        offset += table.data.size();
        offset = (offset + 3) & ~static_cast<std::size_t>(3);
    }
    for (const Table &table : tables) {
        font.insert(font.end(), table.data.begin(), table.data.end());
        while ((font.size() & 3) != 0) {
            font.push_back(0);
        }
    }
    target.assign(font.begin(), font.end());
}

bool TrueTypeFace::buildSubset(const std::vector<char32_t> &codepoints, GlyphSubset &subset,
                               std::string &fontFile, std::string &error) const {
    if (!isLoaded()) {
        error = "font is not loaded";
        return false;
    }
    subset = GlyphSubset();
    for (char32_t codepoint : codepoints) {
        const unsigned glyph = glyphFor(codepoint);
        subset.cidByCodepoint[codepoint] = glyph;
        if (glyph != kMissingGlyph) {
            subset.glyphs.push_back(glyph);
        }
    }
    if (subset.glyphs.empty()) {
        error = "no glyph matched the requested text";
        return false;
    }
    subset.glyphs.insert(subset.glyphs.begin(), kMissingGlyph);
    expandClosure(subset.glyphs);
    for (std::size_t index = 0; index < subset.glyphs.size(); ++index) {
        subset.newIndex[subset.glyphs[index]] = static_cast<unsigned>(index);
    }
    for (auto &entry : subset.cidByCodepoint) {
        entry.second = newIndexFor(subset, entry.second);
    }
    const std::vector<std::uint8_t> glyf = buildGlyf(subset);
    const std::vector<std::uint8_t> loca = buildLoca(subset, glyf);
    std::vector<Table> tables;
    tables.push_back({"OS/2", buildOs2()});
    tables.push_back({"cvt ", buildEmptyTable()});
    tables.push_back({"fpgm", buildEmptyTable()});
    tables.push_back({"glyf", glyf});
    tables.push_back({"head", buildHead(loca.size())});
    tables.push_back({"hhea", std::vector<std::uint8_t>(m_data.begin() + m_hhea.offset,
                                                        m_data.begin() + m_hhea.offset +
                                                                m_hhea.length)});
    tables.push_back({"hmtx", buildHmtx(subset)});
    tables.push_back({"loca", loca});
    tables.push_back({"maxp", buildMaxp(static_cast<int>(subset.glyphs.size()))});
    tables.push_back({"name", buildName()});
    tables.push_back({"post", buildPost()});
    tables.push_back({"prep", buildEmptyTable()});
    std::sort(tables.begin(), tables.end(),
              [](const Table &left, const Table &right) { return left.tag < right.tag; });
    serializeFont(tables, fontFile);
    subset.empty = false;
    error.clear();
    return true;
}

}
