#include "core/util/StringUtil.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace string_util {

std::string trim(const std::string &text) {
    const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
    auto begin = std::find_if_not(text.begin(), text.end(), isSpace);
    auto end = std::find_if_not(text.rbegin(), text.rend(), isSpace).base();
    if (begin >= end) {
        return std::string();
    }
    return std::string(begin, end);
}

std::vector<std::string> splitLines(const std::string &text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

std::string join(const std::vector<std::string> &parts, const std::string &separator) {
    std::string joined;
    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (index > 0) {
            joined += separator;
        }
        joined += parts[index];
    }
    return joined;
}

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::string removeWhitespace(const std::string &text) {
    std::string stripped;
    stripped.reserve(text.size());
    for (char ch : text) {
        if (std::isspace(static_cast<unsigned char>(ch)) == 0) {
            stripped.push_back(ch);
        }
    }
    return stripped;
}

bool startsWith(const std::string &text, const std::string &prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

std::string replaceAll(std::string text, const std::string &from, const std::string &to) {
    if (from.empty()) {
        return text;
    }
    std::size_t position = 0;
    while ((position = text.find(from, position)) != std::string::npos) {
        text.replace(position, from.size(), to);
        position += to.size();
    }
    return text;
}

std::string repeated(const std::string &unit, int count) {
    std::string result;
    for (int index = 0; index < count; ++index) {
        result += unit;
    }
    return result;
}

}
