#ifndef DIZITABFORGE_CORE_PARSE_SCOREPARSER_H
#define DIZITABFORGE_CORE_PARSE_SCOREPARSER_H

#include <string>

#include "core/parse/ScoreToken.h"

namespace dizi {

class ScoreParser {
public:
    bool parse(const TokenList &input, TokenList &resolved);

    const std::string &error() const;

private:
    struct Modifiers {
        int accidental = 0;
        int octave = 0;
    };

    bool combine(const TokenList &input, TokenList &resolved);

    bool applyBeat(const ScoreToken &token, Modifiers &modifiers, TokenList &resolved) const;

    bool applyHold(const ScoreToken &token, Modifiers &modifiers, TokenList &resolved) const;

    bool applyRest(const ScoreToken &token, Modifiers &modifiers, TokenList &resolved) const;

    bool applyAccidental(const ScoreToken &token, Modifiers &modifiers, TokenList &resolved) const;

    bool applyOctave(const ScoreToken &token, Modifiers &modifiers, TokenList &resolved) const;

    bool fail(const ScoreToken &token, const std::string &reason);

    std::string m_error;
};

}

#endif
