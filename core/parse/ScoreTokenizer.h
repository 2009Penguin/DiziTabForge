#ifndef DIZITABFORGE_CORE_PARSE_SCORETOKENIZER_H
#define DIZITABFORGE_CORE_PARSE_SCORETOKENIZER_H

#include <cstddef>
#include <string>

#include "core/parse/ScoreToken.h"

namespace dizi {

class ScoreTokenizer {
public:
    bool tokenize(const std::string &text, TokenList &target);

    std::size_t skippedCount() const;

private:
    bool readToken(char symbol, std::size_t position, ScoreToken &target) const;

    std::size_t m_skipped = 0;
};

}

#endif
