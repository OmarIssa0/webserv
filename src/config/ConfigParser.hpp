#pragma once
#include <fstream>
#include <sstream>
#include <vector>
#include "ServerConfig.hpp"

class ConfigParser {
    std::string               file;
    std::vector<ServerConfig> servers;
    std::string               error;

   public:
    ConfigParser(const std::string& f);
    bool                      parse();
    std::vector<ServerConfig> getServers() const;
    std::string               getError() const;

   private:
    void        parseServer(std::ifstream& f);
    void        parseLocation(std::ifstream& f, ServerConfig& srv, const std::string& header);
    void        parseServerDirective(const std::string& l, ServerConfig& srv);
    void        parseLocationDirective(const std::string& l, LocationConfig& loc);
    std::string clean(std::string v);
    std::string trim(const std::string& s);
};
