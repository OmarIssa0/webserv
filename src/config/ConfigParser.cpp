#include "ConfigParser.hpp"
ConfigParser::ConfigParser(const std::string& f) : file(f) {}

std::string ConfigParser::clean(std::string v) {
    if (!v.empty() && v[v.size() - 1] == ';')
        v.erase(v.size() - 1);
    return v;
}

std::string ConfigParser::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return std::string();
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool ConfigParser::parse() {
    std::ifstream f(file.c_str());
    if (!f.is_open()) {
        error = "Config open failed";
        return false;
    }
    std::string line;
    while (std::getline(f, line)) {
        std::string t = trim(line);
        if (t.find("server") != std::string::npos)
            parseServer(f);
    }
    return true;
}

void ConfigParser::parseServer(std::ifstream& f) {
    ServerConfig srv;
    std::string  line;

    while (std::getline(f, line)) {
        std::string t = trim(line);
        if (t == "{")
            continue;
        if (t == "}")
            break;

        if (t.find("location") != std::string::npos)
            parseLocation(f, srv, line);
        else
            parseServerDirective(line, srv);
    }
    servers.push_back(srv);
}

void ConfigParser::parseLocation(std::ifstream& f, ServerConfig& srv, const std::string& header) {
    std::stringstream ss(header);
    std::string       tmp, path;
    ss >> tmp >> path;

    LocationConfig loc(path);
    std::string    line;

    while (std::getline(f, line)) {
        std::string t = trim(line);
        if (t == "{")
            continue;
        if (t == "}")
            break;
        parseLocationDirective(line, loc);
    }
    srv.addLocation(loc);
}

void ConfigParser::parseServerDirective(const std::string& l, ServerConfig& srv) {
    std::stringstream ss(l);
    std::string       key, val;
    ss >> key >> val;
    val = clean(val);

    if (key == "listen")
        srv.setPort(std::atoi(val.c_str()));
    else if (key == "server_name")
        srv.setServerName(val);
    else if (key == "root")
        srv.setRoot(val);
}

void ConfigParser::parseLocationDirective(const std::string& l, LocationConfig& loc) {
    std::stringstream ss(l);
    std::string       key, val;
    ss >> key >> val;
    val = clean(val);

    if (key == "root")
        loc.setRoot(val);
    else if (key == "autoindex")
        loc.setAutoIndex(val == "on");
}

std::vector<ServerConfig> ConfigParser::getServers() const {
    return servers;
}
std::string ConfigParser::getError() const {
    return error;
}

