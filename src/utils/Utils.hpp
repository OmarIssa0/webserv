#ifndef UTILS_HPP
#define UTILS_HPP
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Logger.hpp"

std::string toUpperWords(const std::string& str);
std::string toLowerWords(const std::string& str);
std::string trimSpacesComments(const std::string& s);
std::string cleanCharEnd(const std::string& v, char c);
bool        convertFileToLines(std::string file, std::vector<std::string>& lines);
bool        checkAllowedMethods(const std::string& m);
bool        parseKeyValue(const std::string& line, std::string& key, std::vector<std::string>& values);
bool        splitByChar(const std::string& line, std::string& key, std::string& value, char endChar);

template <typename type>
std::string typeToString(type _value) {
    std::stringstream ss;
    ss << _value;
    return ss.str();
}

#endif