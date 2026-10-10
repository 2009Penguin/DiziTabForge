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
    bool onBeat(const ScoreToken &token, Score &target);

    bool onRest(const ScoreToken &token, Score &target);

    bool onHold(const ScoreToken &token, Score &target);

    void appendNote(const ScoreToken &token, Score &target);

    void appendRest(Score &target);

    bool fail(const ScoreToken &token, const std::string &reason);

    std::string m_error;
};

}

#endif
