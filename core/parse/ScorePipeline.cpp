#include "core/parse/ScorePipeline.h"

#include <QDebug>

namespace dizi {

const std::string &ScorePipeline::error() const {
    return m_error;
}

std::size_t ScorePipeline::skippedCount() const {
    return m_tokenizer.skippedCount();
}

bool ScorePipeline::parse(const std::string &notation, Score &target) {
    m_error.clear();
    qDebug().noquote() << "[pipeline] raw notation =" << QString::fromStdString(notation);
    TokenList tokens;
    m_tokenizer.tokenize(notation, tokens);
    if (m_tokenizer.skippedCount() > 0) {
        qWarning().noquote() << "[pipeline] skipped unreadable characters ="
                             << static_cast<int>(m_tokenizer.skippedCount());
    }
    if (tokens.empty()) {
        m_error = "The model returned no playable notation symbols.";
        qWarning().noquote() << "[pipeline]" << QString::fromStdString(m_error);
        return false;
    }
    const QString preview = QString::fromStdString(notation).left(200);
    if (looksLikeNoise(notation)) {
        m_error = QStringLiteral("The model output does not look like numbered notation "
                                 "(rejected symbols present). Preview: %1")
                          .arg(preview)
                          .toStdString();
        qWarning().noquote() << "[pipeline] rejected noisy output:" << preview;
        return false;
    }
    TokenList resolved;
    if (!m_parser.parse(tokens, resolved, notation)) {
        m_error = m_parser.error();
        return false;
    }
    if (!m_interpreter.interpret(resolved, target)) {
        m_error = m_interpreter.error();
        qWarning().noquote() << "[pipeline] interpret failed:"
                             << QString::fromStdString(m_error);
        return false;
    }
    qDebug().noquote() << "[pipeline] parsed notes =" << static_cast<int>(target.size());
    return true;
}

bool ScorePipeline::looksLikeNoise(const std::string &notation) const {
    std::size_t meaningful = 0;
    std::size_t rejected = 0;
    for (char symbol : notation) {
        if (symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r') {
            continue;
        }
        if (isNotationSymbol(symbol)) {
            ++meaningful;
        } else {
            ++rejected;
        }
    }
    if (meaningful == 0) {
        return true;
    }
    return rejected * 4 > meaningful;
}

bool ScorePipeline::isNotationSymbol(char symbol) {
    if (symbol >= '0' && symbol <= '7') {
        return true;
    }
    switch (symbol) {
        case '#':
        case 'b':
        case '-':
        case '\'':
        case ',':
            return true;
        default:
            return false;
    }
}

}
