#include "Utils.hpp"

std::string toUpperWords(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] >= 'a' && result[i] <= 'z')
            result[i] = std::toupper(result[i]);
    }
    return result;
}
std::string toLowerWords(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] >= 'A' && result[i] <= 'Z')
            result[i] = std::tolower(result[i]);
    }
    return result;
}

std::string cleanCharEnd(const std::string& v, char c) {
    if (!v.empty() && v[v.size() - 1] == c)
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

bool splitByChar(const std::string& line, std::string& key, std::string& value, char endChar) {
    size_t pos = line.find(endChar);
    if (pos == std::string::npos)
        return false;
    key   = line.substr(0, pos);
    value = line.substr(pos + 1);
    return true;
}

bool parseKeyValue(const std::string& line, std::string& key, std::vector<std::string>& values) {
    std::stringstream ss(line);
    ss >> key;
    if (key.empty())
        return false;

    std::string v;
    while (ss >> v) {
        values.push_back(cleanCharEnd(v, ';'));
    }
    return !values.empty();
}
size_t convertMaxBodySize(const std::string& maxBody) {
    char              unit = maxBody[maxBody.size() - 1];
    std::stringstream ss(maxBody.substr(0, maxBody.size() - 1));
    size_t            size;
    ss >> size;
    switch (unit) {
        case 'K':
        case 'k':
            return size * 1024;
        case 'M':
        case 'm':
            return size * 1024 * 1024;
        case 'G':
        case 'g':
            return size * 1024 * 1024 * 1024;
        default:
            return size;
    }
}

std::string formatSize(size_t size) {
    if (size < 1024) {
        return typeToString(size);
    } else if (size < 1024 * 1024) {
        return typeToString(size / 1024) + " K";
    } else if (size < 1024 * 1024 * 1024) {
        return typeToString(size / (1024 * 1024)) + " M";
    } else {
        return typeToString(size / (1024 * 1024 * 1024)) + " G";
    }
}