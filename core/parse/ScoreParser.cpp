#include "core/parse/ScoreParser.h"

#include <QDebug>

#include <algorithm>

namespace dizi {

namespace {

constexpr int kSharpSign = 1;
constexpr int kFlatSign = -1;
constexpr char kSpace = ' ';

std::string escapeControl(char symbol) {
    if (symbol == '\n') {
        return "\\n";
    }
    if (symbol == '\r') {
        return "\\r";
    }
    if (symbol == '\t') {
        return "\\t";
    }
    return std::string(1, symbol);
}

std::string markerName(TokenType type) {
    switch (type) {
        case TokenType::Sharp:
            return "'#' (sharp)";
        case TokenType::Flat:
            return "'b' (flat)";
        case TokenType::OctaveUp:
            return "''' (high octave)";
        case TokenType::OctaveDown:
            return "',' (low octave)";
        case TokenType::Hold:
            return "'-' (hold)";
        case TokenType::Rest:
            return "'0' (rest)";
        case TokenType::Beat:
            return "a digit";
    }
    return "token";
}

std::string describeTokens(const TokenList &tokens, std::size_t upto) {
    std::string text;
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (index > 0) {
            text += kSpace;
        }
        if (index == upto) {
            text += ">>>";
        }
        text += tokens[index].text;
        if (index == upto) {
            text += "<<<";
        }
    }
    return text;
}

std::string excerptAround(const std::string &notation, std::size_t position, std::size_t *column) {
    if (notation.empty()) {
        return std::string();
    }
    const std::size_t at = position < notation.size() ? position : notation.size() - 1;
    const std::size_t start = at > 24 ? at - 24 : 0;
    const std::size_t stop = std::min(notation.size(), at + 25);
    std::string excerpt;
    *column = 0;
    for (std::size_t index = start; index < stop; ++index) {
        if (index == at) {
            *column = excerpt.size();
        }
        excerpt += escapeControl(notation[index]);
    }
    if (start > 0) {
        excerpt = "..." + excerpt;
        *column += 3;
    }
    if (stop < notation.size()) {
        excerpt += "...";
    }
    return excerpt;
}

}

const std::string &ScoreParser::error() const {
    return m_error;
}

bool ScoreParser::fail(const ScoreToken &token, const std::string &reason) {
    const std::string offending =
            token.text.empty() ? std::string("(no text)") : escapeControl(token.text.at(0));
    std::string detail = reason + ": found " + markerName(token.type) + " as '" + offending +
                         "' at index " + std::to_string(token.position);
    std::size_t column = 0;
    const std::string excerpt = excerptAround(m_notation, token.position, &column);
    if (!excerpt.empty()) {
        detail += "\n  ..." + excerpt + "\n  ";
        detail += std::string(column, kSpace) + "^";
    }
    detail += "\n  parsed so far: " + describeTokens(m_tokenList, m_tokenIndex);
    m_error = detail;
    qWarning().noquote() << "[parse] failed:" << QString::fromStdString(detail);
    return false;
}

bool ScoreParser::flushPending(TokenList &resolved) {
    if (!m_beatOpen) {
        return true;
    }
    if (!m_lastComplete) {
        return fail(m_last, "An accidental or octave mark must be followed by a note digit");
    }
    resolved.push_back(m_last);
    m_beatOpen = false;
    m_noteComplete = true;
    return true;
}

bool ScoreParser::close(TokenList &resolved) {
    const bool ok = flushPending(resolved);
    m_beatOpen = false;
    return ok;
}

bool ScoreParser::onBeat(const ScoreToken &token, TokenList &resolved) {
    if (!close(resolved)) {
        return false;
    }
    m_last = token;
    m_last.accidental = m_pendingAccidental;
    m_pendingAccidental = 0;
    m_beatOpen = true;
    m_beatAllowed = false;
    m_noteComplete = true;
    m_lastComplete = true;
    m_octave = 0;
    return true;
}

bool ScoreParser::onModifier(const ScoreToken &token, TokenList &resolved) {
    (void) resolved;
    const bool isAccidental = token.type == TokenType::Sharp || token.type == TokenType::Flat;
    if (isAccidental && !m_beatOpen) {
        if (!m_beatAllowed) {
            return fail(token, "An accidental must precede a note digit");
        }
        m_pendingAccidental = token.type == TokenType::Sharp ? kSharpSign : kFlatSign;
        return true;
    }
    if (!m_beatOpen) {
        return fail(token, "An accidental or octave mark must follow a note digit");
    }
    const int step = token.type == TokenType::OctaveUp ? 1 : -1;
    if (m_octave + step > 1 || m_octave + step < -1) {
        return fail(token, "Only one octave mark is supported on each side of the digit");
    }
    m_octave += step;
    m_last.octave = m_octave;
    m_noteComplete = true;
    m_lastComplete = true;
    return true;
}

bool ScoreParser::onHold(const ScoreToken &token, TokenList &resolved) {
    if (!m_noteComplete) {
        return fail(token, "A hold mark must directly follow a note digit");
    }
    if (!flushPending(resolved)) {
        return false;
    }
    resolved.push_back(token);
    m_beatAllowed = true;
    return true;
}

bool ScoreParser::onRest(const ScoreToken &token, TokenList &resolved) {
    if (!close(resolved)) {
        return false;
    }
    ScoreToken rest = token;
    rest.type = TokenType::Rest;
    resolved.push_back(rest);
    m_noteComplete = false;
    m_beatAllowed = true;
    return true;
}

bool ScoreParser::update(const ScoreToken &token, TokenList &resolved) {
    switch (token.type) {
        case TokenType::Beat:
            return onBeat(token, resolved);
        case TokenType::Sharp:
        case TokenType::Flat:
        case TokenType::OctaveUp:
        case TokenType::OctaveDown:
            return onModifier(token, resolved);
        case TokenType::Hold:
            return onHold(token, resolved);
        case TokenType::Rest:
            return onRest(token, resolved);
    }
    return fail(token, "Unsupported notation token");
}

bool ScoreParser::parse(const TokenList &input, TokenList &resolved,
                        const std::string &notation) {
    m_error.clear();
    resolved.clear();
    m_notation = notation;
    m_tokenList = input;
    m_beatOpen = false;
    m_beatAllowed = true;
    m_noteComplete = false;
    m_lastComplete = false;
    m_octave = 0;
    m_pendingAccidental = 0;
    qWarning().noquote() << "[parse] input symbols ="
                         << QString::fromStdString(describeTokens(input, input.size()));
    for (m_tokenIndex = 0; m_tokenIndex < input.size(); ++m_tokenIndex) {
        if (!update(input[m_tokenIndex], resolved)) {
            return false;
        }
    }
    if (!close(resolved)) {
        return false;
    }
    qWarning().noquote() << "[parse] next-note stream ="
                         << QString::fromStdString(describeTokens(resolved, resolved.size()));
    return true;
}

}
