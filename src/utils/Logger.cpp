#include "Logger.hpp"
#include <iostream>

void Logger::info(const char* msg) {
    std::cout << "INFO: " << msg << std::endl;
}

void Logger::error(const char* msg) {
    std::cerr << "\033[31mERROR: " << msg << "\033[0m" << std::endl;
}

void Logger::info(const std::string& msg) {
    info(msg.c_str());
}

void Logger::error(const std::string& msg) {
    error(msg.c_str());
}
