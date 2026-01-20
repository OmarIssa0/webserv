#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP
#include <fstream>
#include <sstream>
#include <vector>
#include "../utils/Logger.hpp"
#include "../utils/Utils.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"

class ConfigParser {
   public:
    ConfigParser(const std::string& f);
    bool                      parse();
    std::vector<ServerConfig> getServers() const;
    //getter
    std::string getHttpClientMaxBody() const;

   private:
    enum Scope { NONE, HTTP, SERVER, LOCATION };
    std::string               file;
    std::string               content_file;
    std::vector<ServerConfig> servers;
    Scope                     scope;
    size_t                    curr_index;
    std::string               httpClientMaxBody;
    std::vector<std::string>  lines;
    bool                      parseHttp();
    bool                      parseServer();
    bool                      parseLocation(ServerConfig& srv, const std::string& header);
    bool                      parseServerDirective(const std::string& l, ServerConfig& srv, bool& hasListen);
    bool                      parseLocationDirective(const std::string& l, LocationConfig& loc);
    bool                      getNextLine(std::string& out);

    bool validate();
};
#endif