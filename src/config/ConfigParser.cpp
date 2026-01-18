#include "ConfigParser.hpp"
#include <cstdlib>

ConfigParser::ConfigParser(const std::string& f) : file(f), scope(NONE), curr_index(0) {
    std::ifstream ff(file.c_str());
    if (!ff.is_open()) {
        std::cerr << "Failed to open file: " << file << std::endl;
        return;
    }

    std::string line;
    std::string current;

    while (std::getline(ff, line)) {
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '{' || c == ';' || c == '}') {
                std::string t = trim(current);
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
        std::string t = trim(current);
        if (!t.empty())
            lines.push_back(t);
        current.clear();
    }
}

bool ConfigParser::getNextLine(std::string& out) {
    if (curr_index >= lines.size())
        return false;
    out = lines[curr_index++];
    return true;
}

std::string ConfigParser::clean(const std::string& v) {
    // is used to remove trailing ';' from directive values
    if (!v.empty() && v[v.size() - 1] == ';')
        return v.substr(0, v.size() - 1);
    return v;
}

std::string ConfigParser::trim(const std::string& s) {
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

bool ConfigParser::parse() {
    std::ifstream f(file.c_str());
    if (!f.is_open())
        return setError("Failed to open config");
    std::string t;
    bool        is_http_defined = false;
    while (getNextLine(t)) {
        if (t.empty() || t[0] == '#')
            continue;

        if (t == "http {") {
            if (is_http_defined)
                return setError("only one http block allowed");
            is_http_defined = true;
            if (scope != NONE)
                return setError("http block in invalid position");
            scope = HTTP;
            if (!parseHttp())
                return false;
            scope = NONE;
            continue;
        } else if (t == "server {") {
            if (scope != NONE)
                return setError("server block in invalid position");
            scope = SERVER;
            if (!parseServer())
                return false;
            scope = NONE;
            continue;
        }
        return setError("Invalid directive: " + t);
    }

    if (servers.empty())
        return setError("No server defined");

    return validate();
}

bool ConfigParser::parseHttp() {
    std::string line;
    while (getNextLine(line)) {
        std::string t = trim(line);
        if (t.empty())
            continue;
        if (t == "}") {
            scope = NONE;
            break;
        }
        if (t == "server {") {
            if (!parseServer())
                return false;
            continue;
        }
        std::stringstream ss(t);
        std::string       key, val;
        ss >> key >> val;
        val = clean(val);

        if (key == "client_max_body_size") {
            if (!httpClientMaxBody.empty())
                return setError("duplicate client_max_body_size");
            httpClientMaxBody = val;
        } else
            return setError("Unknown http directive: " + key);
    }
    if (servers.size() == 0) {
        return setError("No server defined");
    }
    if (scope != NONE)
        return setError("Unexpected end of file, missing '}'");
    return true;
}
bool ConfigParser::parseServer() {
    ServerConfig srv;
    bool         hasListen = false;
    scope                  = SERVER;

    std::string l;
    while (getNextLine(l)) {
        std::string t = trim(l);
        if (t.empty())
            continue;
        if (t == "}") {
            scope = HTTP;
            break;
        }

        if (t.find("location ") == 0) {
            if (!parseLocation(srv, t))
                return false;
            continue;
        }

        if (!parseServerDirective(t, srv, hasListen))
            return false;
    }

    if (!hasListen)
        return setError("listen is required");

    if (srv.getLocations().empty())
        return setError("at least one location is required");

    servers.push_back(srv);
    return true;
}
bool ConfigParser::parseServerDirective(const std::string& l, ServerConfig& srv, bool& hasListen) {
    std::stringstream ss(l);
    std::string       key, val;
    ss >> key >> val;
    val = clean(val);

    if (key == "listen") {
        if (hasListen)
            return setError("duplicate listen");
        size_t c = val.find(':');
        if (c == std::string::npos)
            return setError("invalid listen format");
        srv.setInterface(val.substr(0, c));
        srv.setPort(std::atoi(val.substr(c + 1).c_str()));
        if (srv.getPort() <= 0 || srv.getPort() > 65535)
            return setError("invalid port");
        hasListen = true;
    } else if (key == "root") {
        if (!srv.getRoot().empty())
            return setError("duplicate root");
        srv.setRoot(val);
    } else if (key == "server_name") {
        if (!srv.getServerName().empty())
            return setError("duplicate server_name");
        srv.setServerName(val);
    } else if (key == "index") {
        if (!srv.getIndex().empty())
            return setError("duplicate index");
        srv.setIndex(val);
    } else if (key == "client_max_body_size") {
        if (!srv.getClientMaxBody().empty())
            return setError("duplicate client_max_body_size");
        srv.setClientMaxBody(val);
    } else
        return setError("Unknown server directive: " + key);

    return true;
}

bool ConfigParser::parseLocation(ServerConfig& srv, const std::string& header) {
    std::stringstream ss(header);
    std::string       loc, path, brace;
    ss >> loc >> path >> brace;

    if (brace != "{")
        return setError("Invalid location syntax");
    if (path.empty() || path[0] != '/')
        return setError("Location path required");
    for (size_t i = 0; i < srv.getLocations().size(); i++) {
        if (srv.getLocations()[i].getPath() == path)
            return setError("duplicate location path: " + path);
    }

    LocationConfig locCfg(path);
    scope = LOCATION;

    std::string line;
    while (getNextLine(line)) {
        std::string t = trim(line);
        if (t.empty())
            continue;
        if (t == "}") {
            scope = SERVER;
            break;
        }

        if (!parseLocationDirective(t, locCfg))
            return false;
    }

    srv.addLocation(locCfg);
    return true;
}
bool ConfigParser::parseLocationDirective(const std::string& l, LocationConfig& loc) {
    std::stringstream ss(l);
    std::string       key, val;
    ss >> key >> val;
    val = clean(val);

    if (key == "root") {
        if (!loc.getRoot().empty())
            return setError("duplicate root");
        loc.setRoot(val);
    } else if (key == "autoindex") {
        if (loc.getAutoIndex() != false)
            return setError("duplicate autoindex");
        if (val != "on" && val != "off")
            return setError("invalid autoindex value");
        loc.setAutoIndex(val == "on");
    } else if (key == "index") {
        if (!loc.getIndex().empty())
            return setError("duplicate index");
        loc.setIndex(val);
    } else if (key == "client_max_body_size") {
        if (!loc.getClientMaxBody().empty())
            return setError("duplicate client_max_body_size");
        loc.setClientMaxBody(val);
    } else if (key == "methods") {
        std::string m;
        while (ss >> m) {
            std::vector<std::string> methods = loc.getAllowedMethods();
            for (size_t i = 0; i < methods.size(); i++) {
                if (methods[i] == m)
                    return setError("duplicate method: " + m);
            }
            loc.addAllowedMethod(m);
        }
    } else {
        return setError("Unknown location directive: " + key);
    }

    return true;
}

bool ConfigParser::validate() {
    for (size_t i = 0; i < servers.size(); i++) {
        ServerConfig& s = servers[i];

        if (!httpClientMaxBody.empty() && s.getClientMaxBody().empty())
            s.setClientMaxBody(httpClientMaxBody);
        if (httpClientMaxBody.empty() && s.getClientMaxBody().empty()){
            httpClientMaxBody = "1M";
            s.setClientMaxBody("1M");
        }
        for (size_t j = 0; j < s.getLocations().size(); j++) {
            LocationConfig& l = s.getLocations()[j];
            if (l.getRoot().empty()) {
                if (s.getRoot().empty())
                    return setError("location has no root and server has no root");
                l.setRoot(s.getRoot());
            }
            if (l.getAllowedMethods().empty())
                l.addAllowedMethod("GET");
            if (l.getClientMaxBody().empty())
                l.setClientMaxBody(s.getClientMaxBody());
        }
    }
    return true;
}

bool ConfigParser::setError(const std::string& msg) {
    std::stringstream ss;
    ss << msg;
    error = ss.str();
    return false;
}

std::vector<ServerConfig> ConfigParser::getServers() const {
    return servers;
}
std::string ConfigParser::getError() const {
    return error;
}

std::string ConfigParser::getHttpClientMaxBody() const {
    return httpClientMaxBody;
}