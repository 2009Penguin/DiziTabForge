#ifndef DIZITABFORGE_CORE_PARSE_SCOREINTERPRETER_H
#define DIZITABFORGE_CORE_PARSE_SCOREINTERPRETER_H

#include <string>

#include "core/model/Note.h"
#include "core/parse/ScoreToken.h"

namespace dizi {

class ScoreInterpreter {
public:
    bool interpret(const TokenList &resolved, Score &target);

    const std::string &error() const;

private:
    struct Cursor {
        int number = 0;
        int accidental = 0;
        int octave = 0;
    };

    using Handler = bool (ScoreInterpreter::*)(const ScoreToken &);

    bool onAccidental(const ScoreToken &token);

    bool onOctave(const ScoreToken &token);

    bool onHold(const ScoreToken &token);

    bool handleModifier(const ScoreToken &token);

    void appendNote(const ScoreToken &token, Score &target);

    void appendRest(Score &target);

    bool fail(const ScoreToken &token, const std::string &reason);

    void reset();

    std::string m_error;
    Cursor m_cursor;
    bool m_noteOpen = false;
};

}

#endif
