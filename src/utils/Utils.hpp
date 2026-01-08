#ifndef UTILS_HPP
#define UTILS_HPP

#include <sstream>
#include <string>

// Template function to convert any type to string
// Must be in header because it's a template
template <typename T>
std::string toString(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

#endif
