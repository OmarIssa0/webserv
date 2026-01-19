#include "Utils.hpp"
std::string toUpperWords(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] >= 'a' && result[i] <= 'z')
            result[i] = std::toupper(result[i]);
    }
    return result;
}