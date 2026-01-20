#include "LocationConfig.hpp"
LocationConfig::LocationConfig(const std::string& p)
    : path(p),
      root(""),
      autoIndex(false),
      indexes(),
      uploadEnabled(false),
      uploadPath(""),
      cgiEnabled(false),
      cgiPath(""),
      cgiExtension(""),
      redirect(""),
      allowedMethods() {}
LocationConfig::LocationConfig(const std::string& p, const std::string& r)
    : path(p),
      root(r),
      autoIndex(false),
      indexes(),
      uploadEnabled(false),
      uploadPath(""),
      cgiEnabled(false),
      cgiPath(""),
      cgiExtension(""),
      redirect(""),
      allowedMethods(std::vector<std::string>(1, "GET")) {}

LocationConfig::~LocationConfig() {
    allowedMethods.clear();
}
// setters
void LocationConfig::setRoot(const std::string& r) {
    root = r;
}
void LocationConfig::setAutoIndex(bool v) {
    autoIndex = v;
}
void LocationConfig::setIndexes(const std::vector<std::string>& i) {
    indexes = i;
}
void LocationConfig::setUploadEnabled(bool v) {
    uploadEnabled = v;
}
void LocationConfig::setUploadPath(const std::string& p) {
    uploadPath = p;
}
void LocationConfig::setCgiEnabled(bool v) {
    cgiEnabled = v;
}
void LocationConfig::setCgiPath(const std::string& p) {
    cgiPath = p;
}
void LocationConfig::setCgiExtension(const std::string& e) {
    cgiExtension = e;
}
void LocationConfig::setRedirect(const std::string& r) {
    redirect = r;
}

void LocationConfig::setClientMaxBody(const std::string& c) {
    clientMaxBody = c;
}

void LocationConfig::addAllowedMethod(const std::string& m) {
    allowedMethods.push_back(m);
}

// getters
std::string LocationConfig::getPath() const {
    return path;
}
std::string LocationConfig::getRoot() const {
    return root;
}
bool LocationConfig::getAutoIndex() const {
    return autoIndex;
}
bool LocationConfig::getUploadEnabled() const {
    return uploadEnabled;
}
std::string LocationConfig::getUploadPath() const {
    return uploadPath;
}
bool LocationConfig::getCgiEnabled() const {
    return cgiEnabled;
}
std::string LocationConfig::getCgiPath() const {
    return cgiPath;
}
std::string LocationConfig::getCgiExtension() const {
    return cgiExtension;
}
std::string LocationConfig::getRedirect() const {
    return redirect;
}
std::vector<std::string> LocationConfig::getAllowedMethods() const {
    return allowedMethods;
}
std::string LocationConfig::getClientMaxBody() const {
    return clientMaxBody;
}
std::vector<std::string> LocationConfig::getIndexes() const {
    return indexes;
}