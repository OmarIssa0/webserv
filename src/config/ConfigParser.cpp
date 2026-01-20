#include "ConfigParser.hpp"
#include <cstdlib>

ConfigParser::ConfigParser(const std::string& f) : file(f), scope(NONE), curr_index(0) {
    if (!convertFileToLines(file, lines))
        Logger::error("Failed to read configuration file: " + file);
}

bool ConfigParser::getNextLine(std::string& out) {
    if (curr_index >= lines.size())
        return false;
    out = lines[curr_index++];
    return true;
}

bool ConfigParser::parse() {
    std::string line;
    bool        is_http_defined = false;
    while (getNextLine(line)) {
        if (line.empty())
            continue;
        if (line == "http {") {
            if (is_http_defined)
                return Logger::error("only one http block allowed");
            is_http_defined = true;
            if (scope != NONE)
                return Logger::error("http block in invalid position");
            scope = HTTP;
            if (!parseHttp())
                return false;
            scope = NONE;
            continue;
        } else if (line == "server {") {
            if (scope != NONE)
                return Logger::error("server block in invalid position");
            scope = SERVER;
            if (!parseServer())
                return false;
            scope = NONE;
            continue;
        }
        return Logger::error("Invalid directive: " + line);
    }

    if (servers.empty())
        return Logger::error("No server defined");

    return validate();
}

bool ConfigParser::parseHttp() {
    std::string line;
    while (getNextLine(line)) {
        std::string t = trimSpacesComments(line);
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
        std::string              key;
        std::vector<std::string> values;
        if (!parseKeyValue(t, key, values))
            return Logger::error("invalid http directive: " + t);

        if (key == "client_max_body_size") {
            if (!httpClientMaxBody.empty())
                return Logger::error("duplicate client_max_body_size");
            httpClientMaxBody = values[0];
        } else
            return Logger::error("Unknown http directive: " + key);
    }
    if (servers.size() == 0) {
        return Logger::error("No server defined");
    }
    if (scope != NONE)
        return Logger::error("Unexpected end of file, missing '}' in http block");
    return true;
}
bool ConfigParser::parseServer() {
    ServerConfig srv;
    bool         hasListen = false;
    scope                  = SERVER;

    std::string l;
    while (getNextLine(l)) {
        std::string t = trimSpacesComments(l);
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
        return Logger::error("listen is required");

    if (srv.getLocations().empty())
        return Logger::error("at least one location is required");

    servers.push_back(srv);
    if (scope != HTTP)
        return Logger::error("Unexpected end of file, missing '}' in server block");
    return true;
}
bool ConfigParser::parseServerDirective(const std::string& l, ServerConfig& srv, bool& hasListen) {
    std::string              key;
    std::vector<std::string> values;
    if (!parseKeyValue(l, key, values))
        return Logger::error("invalid server directive: " + l);

    if (key == "listen") {
        if (hasListen)
            return Logger::error("duplicate listen");
        if (values.size() != 1)
            return Logger::error("listen takes exactly one value");
        size_t c = values[0].find(':');
        if (c == std::string::npos)
            return Logger::error("invalid listen format");
        srv.setInterface(values[0].substr(0, c));
        srv.setPort(std::atoi(values[0].substr(c + 1).c_str()));
        if (srv.getPort() <= 0 || srv.getPort() > 65535)
            return Logger::error("invalid port");
        hasListen = true;
    } else if (key == "root") {
        if (!srv.getRoot().empty())
            return Logger::error("duplicate root");
        if (values.size() != 1)
            return Logger::error("root takes exactly one value");
        srv.setRoot(values[0]);
    } else if (key == "server_name") {
        if (!srv.getServerName().empty())
            return Logger::error("duplicate server_name");
        if (values.size() != 1)
            return Logger::error("server_name takes exactly one value");
        srv.setServerName(values[0]);
    } else if (key == "index") {
        if (!srv.getIndexes().empty())
            return Logger::error("duplicate index");
        srv.setIndexes(values);
    } else if (key == "client_max_body_size") {
        if (!srv.getClientMaxBody().empty())
            return Logger::error("duplicate client_max_body_size");
        if (values.size() != 1)
            return Logger::error("client_max_body_size takes exactly one value");
        srv.setClientMaxBody(values[0]);
    } else
        return Logger::error("Unknown server directive: " + key);

    return true;
}

bool ConfigParser::parseLocation(ServerConfig& srv, const std::string& header) {
    std::string              loc;
    std::vector<std::string> values;
    if (!parseKeyValue(header, loc, values))
        return Logger::error("invalid location syntax");
    if (values.size() != 2 || values[1] != "{")
        return Logger::error("Invalid location syntax");
    if (values.empty() || values[0][0] != '/')
        return Logger::error("Location path required");
    for (size_t i = 0; i < srv.getLocations().size(); i++) {
        if (srv.getLocations()[i].getPath() == values[0])
            return Logger::error("duplicate location path: " + values[0]);
    }

    LocationConfig locCfg(values[0]);
    scope = LOCATION;

    std::string line;
    while (getNextLine(line)) {
        std::string t = trimSpacesComments(line);
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
    if (scope != SERVER)
        return Logger::error("Unexpected end of file, missing '}' in location block");
    return true;
}
bool ConfigParser::parseLocationDirective(const std::string& l, LocationConfig& loc) {
    std::string              key;
    std::vector<std::string> values;
    if (!parseKeyValue(l, key, values))
        return Logger::error("invalid location directive: " + l);
    if (key == "root") {
        if (!loc.getRoot().empty())
            return Logger::error("duplicate root");
        if (values.size() != 1)
            return Logger::error("root takes exactly one value");
        loc.setRoot(values[0]);
    } else if (key == "autoindex") {
        if (loc.getAutoIndex() != false)
            return Logger::error("duplicate autoindex");
        if (values.size() != 1)
            return Logger::error("autoindex takes exactly one value");
        if (values[0] != "on" && values[0] != "off")
            return Logger::error("invalid autoindex value");
        loc.setAutoIndex(values[0] == "on");
    } else if (key == "index") {
        if (!loc.getIndexes().empty())
            return Logger::error("duplicate index");
        loc.setIndexes(values);
    } else if (key == "client_max_body_size") {
        if (!loc.getClientMaxBody().empty())
            return Logger::error("duplicate client_max_body_size");
        if (values.size() != 1)
            return Logger::error("client_max_body_size takes exactly one value");
        loc.setClientMaxBody(values[0]);
    } else if (key == "methods") {
        for (size_t i = 0; i < values.size(); i++) {
            values[i] = toUpperWords(values[i]);
            if (!checkAllowedMethods(values[i]))
                return Logger::error("invalid method: " + values[i]);
            std::vector<std::string> methods = loc.getAllowedMethods();
            for (size_t j = 0; j < methods.size(); j++) {
                if (methods[j] == values[i])
                    return Logger::error("duplicate method: " + values[i]);
            }
            loc.addAllowedMethod(values[i]);
        }
    } else
        return Logger::error("Unknown location directive: " + key);
    return true;
}

bool ConfigParser::validate() {
    for (size_t i = 0; i < servers.size(); i++) {
        ServerConfig& s = servers[i];

        if (!httpClientMaxBody.empty() && s.getClientMaxBody().empty())
            s.setClientMaxBody(httpClientMaxBody);
        if (httpClientMaxBody.empty() && s.getClientMaxBody().empty()) {
            httpClientMaxBody = "1M";
            s.setClientMaxBody("1M");
        }
        for (size_t j = 0; j < s.getLocations().size(); j++) {
            LocationConfig& l = s.getLocations()[j];
            if (l.getRoot().empty()) {
                if (s.getRoot().empty())
                    return Logger::error("location has no root and server has no root");
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

std::vector<ServerConfig> ConfigParser::getServers() const {
    return servers;
}

std::string ConfigParser::getHttpClientMaxBody() const {
    return httpClientMaxBody;
}
