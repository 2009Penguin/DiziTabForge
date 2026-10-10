#ifndef DIZITABFORGE_CORE_PARSE_SCOREPARSER_H
#define DIZITABFORGE_CORE_PARSE_SCOREPARSER_H

#include <string>

#include "core/parse/ScoreToken.h"

namespace dizi {

class ScoreParser {
public:
    bool parse(const TokenList &input, TokenList &resolved, const std::string &notation);

    const std::string &error() const;

private:
    bool update(const ScoreToken &token, TokenList &resolved);

    bool onBeat(const ScoreToken &token, TokenList &resolved);

    bool onModifier(const ScoreToken &token, TokenList &resolved);

    bool onHold(const ScoreToken &token, TokenList &resolved);

    bool onRest(const ScoreToken &token, TokenList &resolved);

    bool close(TokenList &resolved);

    bool flushPending(TokenList &resolved);

    bool fail(const ScoreToken &token, const std::string &reason);

    std::string m_error;
    std::string m_notation;
    TokenList m_tokenList;
    ScoreToken m_last;
    int m_octave = 0;
    bool m_beatOpen = false;
    bool m_beatAllowed = true;
    bool m_noteComplete = false;
    bool m_lastComplete = false;
    int m_pendingAccidental = 0;
    std::size_t m_tokenIndex = 0;
};

}

#endif
