#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP
#include <fstream>
#include <sstream>
#include <vector>
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"

class ConfigParser {
   public:
    ConfigParser(const std::string& f);
    bool                      parse();
    std::vector<ServerConfig> getServers() const;
    std::string               getError() const;
    //getter
    std::string getHttpClientMaxBody() const;

   private:
    enum Scope { NONE, HTTP, SERVER, LOCATION };

    std::string               file;
    std::string               content_file;
    std::vector<ServerConfig> servers;
    std::string               error;
    Scope                     scope;
    size_t                       curr_index;
    std::string               httpClientMaxBody;
    std::vector<std::string>  lines;

    bool parseHttp();
    bool parseServer();
    bool parseLocation(ServerConfig& srv, const std::string& header);
    bool parseServerDirective(const std::string& l, ServerConfig& srv, bool& hasListen);
    bool parseLocationDirective(const std::string& l, LocationConfig& loc);
    bool getNextLine(std::string& out);

    bool validate();
    bool setError(const std::string& msg);

    std::string trim(const std::string& s);
    std::string clean(const std::string& v);
};
#endif