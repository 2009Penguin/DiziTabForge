#include "core/parse/ScoreInterpreter.h"

namespace dizi {

namespace {

constexpr int kAccidentalNone = 0;
constexpr int kAccidentalSharp = 1;
constexpr int kAccidentalFlat = -1;

std::string describe(const ScoreToken &token) {
    if (!token.text.empty()) {
        return "'" + token.text + "'";
    }
    return "token";
}

}

const std::string &ScoreInterpreter::error() const {
    return m_error;
}

bool ScoreInterpreter::fail(const ScoreToken &token, const std::string &reason) {
    m_error = reason + " at " + describe(token) + " (index " +
              std::to_string(token.position) + ")";
    return false;
}

void ScoreInterpreter::reset() {
    m_cursor = Cursor();
    m_noteOpen = false;
}

bool ScoreInterpreter::onAccidental(const ScoreToken &token) {
    if (!m_noteOpen) {
        return fail(token, "An accidental must follow a beat digit");
    }
    m_cursor.accidental = token.type == TokenType::Sharp ? kAccidentalSharp : kAccidentalFlat;
    return true;
}

bool ScoreInterpreter::onOctave(const ScoreToken &token) {
    if (!m_noteOpen) {
        return fail(token, "An octave mark must follow a beat digit");
    }
    m_cursor.octave += token.type == TokenType::OctaveUp ? 1 : -1;
    if (m_cursor.octave > 1 || m_cursor.octave < -1) {
        return fail(token, "Only one octave mark is supported on each side of the beat");
    }
    m_noteOpen = false;
    return true;
}

bool ScoreInterpreter::onHold(const ScoreToken &token) {
    if (!m_noteOpen) {
        return fail(token, "A hold mark must follow a beat digit");
    }
    return true;
}

bool ScoreInterpreter::interpret(const TokenList &resolved, Score &target) {
    m_error.clear();
    target.clear();
    reset();
    for (const ScoreToken &token : resolved) {
        if (token.type == TokenType::Hold) {
            if (target.empty() || !onHold(token)) {
                return false;
            }
            ++target.back().beats;
            continue;
        }
        if (target.empty() || !m_noteOpen) {
            if (token.type == TokenType::Rest) {
                appendRest(target);
                continue;
            }
            if (token.type != TokenType::Beat) {
                return fail(token, "A beat digit is required here");
            }
            appendNote(token, target);
            continue;
        }
        if (token.type == TokenType::OctaveDown) {
            return fail(token, "A lower octave mark cannot follow a higher octave mark");
        }
        if (!handleModifier(token)) {
            return false;
        }
    }
    return true;
}

void ScoreInterpreter::appendNote(const ScoreToken &token, Score &target) {
    Note note;
    note.number = token.value;
    note.octave = m_cursor.octave;
    note.rest = false;
    note.beats = 1;
    note.accidental = static_cast<Accidental>(m_cursor.accidental);
    target.push_back(note);
    m_noteOpen = true;
    m_cursor.accidental = kAccidentalNone;
}

void ScoreInterpreter::appendRest(Score &target) {
    Note rest;
    rest.octave = m_cursor.octave;
    rest.rest = true;
    rest.beats = 1;
    target.push_back(rest);
    m_noteOpen = false;
    m_cursor.accidental = kAccidentalNone;
}

bool ScoreInterpreter::handleModifier(const ScoreToken &token) {
    if (token.type == TokenType::Sharp || token.type == TokenType::Flat) {
        return onAccidental(token);
    }
    if (token.type == TokenType::OctaveUp) {
        return onOctave(token);
    }
    return fail(token, "A beat digit is required here");
}

}
