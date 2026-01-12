#include "LocationConfig.hpp"
LocationConfig::LocationConfig() : autoindex(false) {}
LocationConfig::LocationConfig(const std::string& p) : path(p), autoindex(false) {}

void LocationConfig::setRoot(const std::string& r) {
    root = r;
}
void LocationConfig::setAutoIndex(bool v) {
    autoindex = v;
}

std::string LocationConfig::getPath() const {
    return path;
}
std::string LocationConfig::getRoot() const {
    return root;
}
bool LocationConfig::getAutoIndex() const {
    return autoindex;
}