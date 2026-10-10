#ifndef DIZITABFORGE_CORE_PARSE_SCORETOKEN_H
#define DIZITABFORGE_CORE_PARSE_SCORETOKEN_H

#include <string>
#include <vector>

namespace dizi {

enum class TokenType {
    Beat,
    Rest,
    Hold,
    OctaveUp,
    OctaveDown,
    Sharp,
    Flat
};

using TokenList = std::vector<ScoreToken>;

struct ScoreToken {
    TokenType type = TokenType::Beat;
    int value = 0;
    std::size_t position = 0;
    std::string text;
};

}

#endif
