#ifndef DIZITABFORGE_CORE_UTIL_STRINGUTIL_H
#define DIZITABFORGE_CORE_UTIL_STRINGUTIL_H

#include <string>
#include <vector>

namespace string_util {

std::string trim(const std::string &text);

std::vector<std::string> splitLines(const std::string &text);

std::string join(const std::vector<std::string> &parts, const std::string &separator);

std::string toLower(std::string text);

std::string removeWhitespace(const std::string &text);

bool startsWith(const std::string &text, const std::string &prefix);

std::string replaceAll(std::string text, const std::string &from, const std::string &to);

std::string repeated(const std::string &unit, int count);

}

#endif
