#ifndef DIZITABFORGE_CORE_RENDER_EMBEDDEDFONT_H
#define DIZITABFORGE_CORE_RENDER_EMBEDDEDFONT_H

#include <map>
#include <string>
#include <vector>

namespace dizi {

struct EmbeddedFont {
    std::string name;
    std::string postScriptName;
    std::string program;
    std::map<char32_t, unsigned> cidByCodepoint;
    std::vector<std::pair<char32_t, unsigned>> unicodeByCid;
    std::vector<std::uint16_t> advanceByCid;
    double ascent = 0.0;
    double descent = 0.0;
    int unitsPerEm = 1000;
    bool usable = false;
};

}

#endif
