#ifndef DIZITABFORGE_CORE_PARSE_SCOREPIPELINE_H
#define DIZITABFORGE_CORE_PARSE_SCOREPIPELINE_H

#include <string>

#include "core/model/Note.h"
#include "core/parse/ScoreInterpreter.h"
#include "core/parse/ScoreParser.h"
#include "core/parse/ScoreToken.h"
#include "core/parse/ScoreTokenizer.h"

namespace dizi {

class ScorePipeline {
public:
    bool parse(const std::string &notation, Score &target);

    const std::string &error() const;

private:
    ScoreTokenizer m_tokenizer;
    ScoreParser m_parser;
    ScoreInterpreter m_interpreter;
    std::string m_error;
};

}

#endif
