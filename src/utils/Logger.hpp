#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <iostream>
#include <string>
class Logger {
   public:
    static bool info(const std::string& message);
    static bool error(const std::string& message);
};
#endif