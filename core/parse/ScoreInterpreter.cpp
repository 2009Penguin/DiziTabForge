#include "core/parse/ScoreInterpreter.h"

#include <QDebug>

namespace dizi {

namespace {

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
    m_error = reason + ": found " + describe(token) + " at index " +
              std::to_string(token.position);
    qWarning().noquote() << "[interpreter] failed:" << QString::fromStdString(m_error);
    return false;
}

void ScoreInterpreter::appendNote(const ScoreToken &token, Score &target) {
    Note note;
    note.number = token.value;
    note.octave = token.octave;
    note.accidental = static_cast<Accidental>(token.accidental);
    note.rest = false;
    note.beats = 1;
    target.push_back(note);
}

void ScoreInterpreter::appendRest(Score &target) {
    Note rest;
    rest.rest = true;
    rest.beats = 1;
    target.push_back(rest);
}

bool ScoreInterpreter::onBeat(const ScoreToken &token, Score &target) {
    appendNote(token, target);
    return true;
}

bool ScoreInterpreter::onRest(const ScoreToken &token, Score &target) {
    (void) token;
    appendRest(target);
    return true;
}

bool ScoreInterpreter::onHold(const ScoreToken &token, Score &target) {
    if (target.empty()) {
        return fail(token, "A hold mark must follow a note digit");
    }
    if (target.back().rest) {
        return fail(token, "A hold mark cannot extend a rest");
    }
    ++target.back().beats;
    return true;
}

bool ScoreInterpreter::interpret(const TokenList &resolved, Score &target) {
    m_error.clear();
    target.clear();
    qDebug().noquote() << "[interpreter] symbols =" << static_cast<int>(resolved.size());
    for (const ScoreToken &token : resolved) {
        bool ok = false;
        switch (token.type) {
            case TokenType::Beat:
                ok = onBeat(token, target);
                break;
            case TokenType::Rest:
                ok = onRest(token, target);
                break;
            case TokenType::Hold:
                ok = onHold(token, target);
                break;
            default:
                return fail(token, "Unexpected mark reached the interpreter");
        }
        if (!ok) {
            return false;
        }
    }
    qDebug().noquote() << "[interpreter] notes =" << static_cast<int>(target.size());
    return true;
}

}
