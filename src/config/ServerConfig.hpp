#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP
#include <iostream>
#include <vector>
#include "LocationConfig.hpp"
class ServerConfig {
   public:
    ServerConfig();
    ~ServerConfig();
    void setPort(int port);
    void setServerName(const std::string& name);
    void setRoot(const std::string& root);
    void addLocation(const LocationConfig& loc);
    void indexLocations();

    //getters
    int                         getPort() const;
    std::string getServerName() const;
    std::string getRoot() const;
    std::vector<LocationConfig> getLocations() const;

   private:
    int                         port;
    std::string                 serverName;
    std::string                 root;
    std::vector<LocationConfig> locations;
};
#endif