#include "core/parse/ScoreTokenizer.h"

#include <QDebug>

#include <cctype>

namespace dizi {

namespace {

bool symbolToToken(char symbol, TokenType &target) {
    switch (symbol) {
        case '\'':
            target = TokenType::OctaveUp;
            return true;
        case ',':
            target = TokenType::OctaveDown;
            return true;
        case '#':
            target = TokenType::Sharp;
            return true;
        case 'b':
            target = TokenType::Flat;
            return true;
        case '-':
            target = TokenType::Hold;
            return true;
        default:
            return false;
    }
}

std::string describeSymbol(char symbol) {
    return std::string(1, symbol);
}

}

bool ScoreTokenizer::readToken(char symbol, std::size_t position, ScoreToken &target) const {
    const bool isDigit = std::isdigit(static_cast<unsigned char>(symbol)) != 0;
    if (isDigit && symbol != '0') {
        const int beat = symbol - '0';
        if (beat < 1 || beat > 7) {
            return false;
        }
        target.type = TokenType::Beat;
        target.value = beat;
    } else if (symbol == '0') {
        target.type = TokenType::Rest;
        target.value = 0;
    } else {
        TokenType type = TokenType::Beat;
        if (!symbolToToken(symbol, type)) {
            return false;
        }
        target.type = type;
        target.value = 0;
    }
    target.position = position;
    target.text = describeSymbol(symbol);
    return true;
}

bool ScoreTokenizer::tokenize(const std::string &text, TokenList &target) {
    target.clear();
    m_skipped = 0;
    for (std::size_t position = 0; position < text.size(); ++position) {
        const char symbol = text[position];
        if (std::isspace(static_cast<unsigned char>(symbol)) != 0) {
            continue;
        }
        ScoreToken token;
        if (!readToken(symbol, position, token)) {
            ++m_skipped;
            continue;
        }
        target.push_back(token);
    }
    qDebug().noquote() << "[tokenizer] symbols =" << static_cast<int>(target.size())
                       << " skipped =" << static_cast<int>(m_skipped);
    return true;
}

std::size_t ScoreTokenizer::skippedCount() const {
    return m_skipped;
}

}
