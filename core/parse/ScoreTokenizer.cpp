#include "core/parse/ScoreTokenizer.h"

#include <cctype>
#include <utility>

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
        case '0':
            target = TokenType::Rest;
            return true;
        default:
            return false;
    }
}

std::string describeSymbol(char symbol) {
    return std::string(1, symbol);
}

}

bool ScoreTokenizer::readSymbol(char symbol, std::size_t position, ScoreToken &target,
                                std::string &error) const {
    TokenType type = TokenType::Beat;
    if (!symbolToToken(symbol, type)) {
        error = "Unexpected character '" + describeSymbol(symbol) + "' at index " +
                std::to_string(position);
        return false;
    }
    target.type = type;
    target.position = position;
    target.text = describeSymbol(symbol);
    target.value = 0;
    return true;
}

bool ScoreTokenizer::readBeat(char symbol, std::size_t position, ScoreToken &target,
                              std::string &error) const {
    if (std::isdigit(static_cast<unsigned char>(symbol)) == 0) {
        error = "Expected a digit at index " + std::to_string(position);
        return false;
    }
    const int beat = symbol - '0';
    if (beat < 1 || beat > 7) {
        error = "Pitch digit '" + describeSymbol(symbol) + "' at index " +
                std::to_string(position) + " is outside the range 1-7";
        return false;
    }
    target.type = TokenType::Beat;
    target.value = beat;
    target.position = position;
    target.text = describeSymbol(symbol);
    return true;
}

bool ScoreTokenizer::tokenize(const std::string &text, TokenList &target,
                              std::string &error) const {
    target.clear();
    error.clear();
    for (std::size_t position = 0; position < text.size(); ++position) {
        const char symbol = text[position];
        if (std::isspace(static_cast<unsigned char>(symbol)) != 0) {
            continue;
        }
        ScoreToken token;
        if (std::isdigit(static_cast<unsigned char>(symbol)) != 0) {
            if (!readBeat(symbol, position, token, error)) {
                return false;
            }
        } else if (!readSymbol(symbol, position, token, error)) {
            return false;
        }
        target.push_back(token);
    }
    return true;
}

}
