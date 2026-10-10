#ifndef DIZITABFORGE_CORE_PARSE_SCORETOKENIZER_H
#define DIZITABFORGE_CORE_PARSE_SCORETOKENIZER_H

#include <string>

#include "core/parse/ScoreToken.h"

namespace dizi {

class ScoreTokenizer {
public:
    bool tokenize(const std::string &text, TokenList &target, std::string &error) const;

private:
    bool readSymbol(char symbol, std::size_t position, ScoreToken &target,
                    std::string &error) const;

    bool readBeat(char symbol, std::size_t position, ScoreToken &target,
                  std::string &error) const;

    bool readRest(char symbol, std::size_t position, ScoreToken &target,
                  std::string &error) const;
};

}

#endif
