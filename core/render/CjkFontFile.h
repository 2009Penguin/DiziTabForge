#ifndef DIZITABFORGE_CORE_RENDER_CJKFONTFILE_H
#define DIZITABFORGE_CORE_RENDER_CJKFONTFILE_H

#include <cstdint>
#include <string>
#include <vector>

#include "core/render/EmbeddedFont.h"
#include "core/render/TrueTypeFace.h"

namespace dizi {

class CjkFontFile {
public:
    bool open(const std::string &path);

    bool openFromMemory(const std::vector<std::uint8_t> &data, const std::string &origin);

    bool isOpen() const;

    const std::string &origin() const;

    bool buildFor(const std::vector<char32_t> &codepoints, EmbeddedFont &target,
                  std::string &error) const;

private:
    TrueTypeFace m_face;
    std::string m_origin;
};

}

#endif
