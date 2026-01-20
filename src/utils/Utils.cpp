#include "Utils.hpp"

std::string toUpperWords(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] >= 'a' && result[i] <= 'z')
            result[i] = std::toupper(result[i]);
    }
    return result;
}

std::string cleanSemicolon(const std::string& v) {
    if (!v.empty() && v[v.size() - 1] == ';')
        return v.substr(0, v.size() - 1);
    return v;
}

std::string trimSpacesComments(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos || s[start] == '#')
        return "";
    size_t end = s.find('#', start);
    if (end != std::string::npos)
        end--;
    else
        end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool convertFileToLines(std::string file, std::vector<std::string>& lines) {
    std::ifstream ff(file.c_str());
    if (!ff.is_open()) {
        return false;
    }

    std::string line;
    std::string current;

    while (std::getline(ff, line)) {
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '{' || c == ';' || c == '}') {
                std::string t = trimSpacesComments(current);
                if (!t.empty())
                    lines.push_back(t + ((c == '{') ? " {" : (c == ';') ? ";" : ""));
                else if (c == '{')
                    lines.push_back("{");
                if (c == '}')
                    lines.push_back("}");
                current.clear();
            } else {
                current += c;
            }
        }
        std::string t = trimSpacesComments(current);
        if (!t.empty())
            lines.push_back(t);
        current.clear();
    }
    ff.close();
    return true;
}

bool checkAllowedMethods(const std::string& m) {
    const std::string methods[] = {"GET", "POST", "DELETE", "PUT", "PATCH", "HEAD", "OPTIONS"};
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); i++) {
        if (methods[i] == m)
            return true;
    }
    return false;
}

bool parseKeyValue(const std::string& line, std::string& key, std::vector<std::string>& values) {
    std::stringstream ss(line);
    ss >> key;
    if (key.empty())
        return false;

    std::string v;
    while (ss >> v) {
        values.push_back(cleanSemicolon(v));
    }
    return !values.empty();
}
