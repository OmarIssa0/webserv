#include "ServerConfig.hpp"
ServerConfig::ServerConfig() : port(-1), interface(""), locations(), serverName(""), root(""), index("index.html"), clientMaxBodySize("") {}
ServerConfig::~ServerConfig() {
    locations.clear();
}
// setters
void ServerConfig::setPort(int p) {
    port = p;
}
void ServerConfig::setInterface(const std::string& i) {
    interface = i;
}
void ServerConfig::setIndex(const std::string& i) {
    index = i;
}
void ServerConfig::setClientMaxBody(const std::string& b) {
    clientMaxBodySize = b;
}
void ServerConfig::setServerName(const std::string& name) {
    serverName = name;
}
void ServerConfig::setRoot(const std::string& r) {
    root = r;
}
void ServerConfig::addLocation(const LocationConfig& loc) {
    locations.push_back(loc);
    if (locations.back().getRoot().empty())
        locations.back().setRoot(root);
    if (locations.back().getIndex().empty())
        locations.back().setIndex(index);
}
//getters
int ServerConfig::getPort() const {
    return port;
}
std::string ServerConfig::getInterface() const {
    return interface;
}
std::vector<LocationConfig>& ServerConfig::getLocations() {
    return locations;
}

const std::vector<LocationConfig>& ServerConfig::getLocations() const {
    return locations;
}
std::string ServerConfig::getIndex() const {
    return index;
}
std::string ServerConfig::getServerName() const {
    return serverName;
}
std::string ServerConfig::getRoot() const {
    return root;
}
std::string ServerConfig::getClientMaxBody() const {
    return clientMaxBodySize;
}