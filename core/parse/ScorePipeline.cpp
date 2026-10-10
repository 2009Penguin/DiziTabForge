#include "core/parse/ScorePipeline.h"

namespace dizi {

const std::string &ScorePipeline::error() const {
    return m_error;
}

bool ScorePipeline::parse(const std::string &notation, Score &target) {
    m_error.clear();
    TokenList tokens;
    if (!m_tokenizer.tokenize(notation, tokens, m_error)) {
        return false;
    }
    TokenList resolved;
    if (!m_parser.parse(tokens, resolved)) {
        m_error = m_parser.error();
        return false;
    }
    if (!m_interpreter.interpret(resolved, target)) {
        m_error = m_interpreter.error();
        return false;
    }
    return true;
}

}
