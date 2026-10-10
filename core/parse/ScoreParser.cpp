#include "core/parse/ScoreParser.h"

#include <array>

namespace dizi {

namespace {

constexpr int kNoAccidental = 0;
constexpr int kSharpSign = 1;
constexpr int kFlatSign = -1;

using Handler = bool (ScoreParser::*)(const ScoreToken &, ScoreParser::Modifiers &,
                                      TokenList &) const;

struct Route {
    TokenType type;
    Handler handler;
};

using Router = std::array<Route, 7>;

const Router &router() {
    static const Router routes{{
            {TokenType::Beat, &ScoreParser::applyBeat},
            {TokenType::Hold, &ScoreParser::applyHold},
            {TokenType::Rest, &ScoreParser::applyRest},
            {TokenType::Sharp, &ScoreParser::applyAccidental},
            {TokenType::Flat, &ScoreParser::applyAccidental},
            {TokenType::OctaveUp, &ScoreParser::applyOctave},
            {TokenType::OctaveDown, &ScoreParser::applyOctave},
    }};
    return routes;
}

const Handler *findHandler(TokenType type) {
    for (const Route &route : router()) {
        if (route.type == type) {
            return &route.handler;
        }
    }
    return nullptr;
}

bool isMarker(TokenType type) {
    return type == TokenType::Sharp || type == TokenType::Flat ||
           type == TokenType::OctaveUp || type == TokenType::OctaveDown;
}

void dropTrailingMarkers(TokenList &resolved) {
    while (!resolved.empty() && isMarker(resolved.back().type)) {
        resolved.pop_back();
    }
}

}

const std::string &ScoreParser::error() const {
    return m_error;
}

bool ScoreParser::fail(const ScoreToken &token, const std::string &reason) {
    m_error = reason + " (near index " + std::to_string(token.position) + ")";
    return false;
}

bool ScoreParser::applyBeat(const ScoreToken &token, Modifiers &modifiers,
                            TokenList &resolved) const {
    dropTrailingMarkers(resolved);
    resolved.push_back(token);
    if (modifiers.accidental != kNoAccidental) {
        ScoreToken accidental;
        accidental.type = modifiers.accidental > 0 ? TokenType::Sharp : TokenType::Flat;
        accidental.position = token.position;
        resolved.push_back(accidental);
    }
    for (int step = 0; step < modifiers.octave; ++step) {
        ScoreToken octave;
        octave.type = TokenType::OctaveUp;
        octave.position = token.position;
        resolved.push_back(octave);
    }
    for (int step = 0; step > modifiers.octave; --step) {
        ScoreToken octave;
        octave.type = TokenType::OctaveDown;
        octave.position = token.position;
        resolved.push_back(octave);
    }
    modifiers = Modifiers();
    return true;
}

bool ScoreParser::applyHold(const ScoreToken &token, Modifiers &modifiers,
                            TokenList &resolved) const {
    if (modifiers.accidental != kNoAccidental || modifiers.octave != 0) {
        return fail(token, "A hold mark cannot follow an accidental or octave mark");
    }
    if (resolved.empty() || isMarker(resolved.back().type)) {
        return fail(token, "A hold mark must follow a note");
    }
    if (resolved.back().type == TokenType::Rest) {
        return fail(token, "A hold mark cannot extend a rest");
    }
    resolved.push_back(token);
    return true;
}

bool ScoreParser::applyRest(const ScoreToken &token, Modifiers &modifiers,
                            TokenList &resolved) const {
    modifiers = Modifiers();
    dropTrailingMarkers(resolved);
    ScoreToken rest = token;
    rest.type = TokenType::Rest;
    rest.value = 1;
    resolved.push_back(rest);
    return true;
}

bool ScoreParser::applyAccidental(const ScoreToken &token, Modifiers &modifiers,
                                  TokenList &resolved) const {
    modifiers.accidental = token.type == TokenType::Sharp ? kSharpSign : kFlatSign;
    return true;
}

bool ScoreParser::applyOctave(const ScoreToken &token, Modifiers &modifiers,
                              TokenList &resolved) const {
    const int step = token.type == TokenType::OctaveUp ? 1 : -1;
    modifiers.octave += step;
    if (modifiers.octave > 2 || modifiers.octave < -2) {
        return fail(token, "An octave mark must stay within two marks of the note");
    }
    return true;
}

bool ScoreParser::combine(const TokenList &input, TokenList &resolved) {
    Modifiers modifiers;
    for (const ScoreToken &token : input) {
        const Handler *handler = findHandler(token.type);
        if (handler == nullptr) {
            return fail(token, "Unsupported notation token");
        }
        if (!(this->**handler)(token, modifiers, resolved)) {
            return false;
        }
    }
    return true;
}

bool ScoreParser::parse(const TokenList &input, TokenList &resolved) {
    m_error.clear();
    resolved.clear();
    return combine(input, resolved);
}

}
