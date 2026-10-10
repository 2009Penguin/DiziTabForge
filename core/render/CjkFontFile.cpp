#include "core/render/CjkFontFile.h"

namespace dizi {

bool CjkFontFile::open(const std::string &path) {
    if (!m_face.load(path)) {
        return false;
    }
    m_origin = path;
    return true;
}

bool CjkFontFile::openFromMemory(const std::vector<std::uint8_t> &data, const std::string &origin) {
    if (!m_face.loadFromMemory(data)) {
        return false;
    }
    m_origin = origin;
    return true;
}

bool CjkFontFile::isOpen() const {
    return m_face.isLoaded();
}

const std::string &CjkFontFile::origin() const {
    return m_origin;
}

bool CjkFontFile::buildFor(const std::vector<char32_t> &codepoints, EmbeddedFont &target,
                           std::string &error) const {
    if (!isOpen()) {
        error = "no Chinese font is loaded";
        return false;
    }
    GlyphSubset subset;
    std::string program;
    if (!m_face.buildSubset(codepoints, subset, program, error)) {
        return false;
    }
    target.name = "DiziTabForgeCJK";
    target.postScriptName = "DiziTabForgeCJK";
    target.program = program;
    target.cidByCodepoint = subset.cidByCodepoint;
    target.unitsPerEm = m_face.metrics().unitsPerEm;
    target.ascent = m_face.metrics().ascent;
    target.descent = m_face.metrics().descent;
    target.advanceByCid.assign(subset.glyphs.size(), 0);
    for (std::size_t slot = 0; slot < subset.glyphs.size(); ++slot) {
        target.advanceByCid[slot] = m_face.advanceFor(subset.glyphs[slot]);
    }
    target.unicodeByCid.clear();
    for (const auto &entry : subset.cidByCodepoint) {
        if (entry.second != 0) {
            target.unicodeByCid.emplace_back(entry.first, entry.second);
        }
    }
    target.usable = true;
    error.clear();
    return true;
}

}
