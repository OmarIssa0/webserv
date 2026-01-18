#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP
#include <iostream>
#include <vector>
#include "LocationConfig.hpp"
class ServerConfig {
   public:
    ServerConfig();
    ~ServerConfig();

    // setters
    void setPort(int p);
    void setInterface(const std::string& i);
    void setIndex(const std::string& i);
    void setClientMaxBody(const std::string& b);
    void setServerName(const std::string& name);
    void setRoot(const std::string& root);
    void addLocation(const LocationConfig& loc);

    //getters
    int                         getPort() const;
    std::string                 getInterface() const;
    std::vector<LocationConfig>& getLocations();
    const std::vector<LocationConfig>& getLocations() const;
    std::string                 getServerName() const;
    std::string                 getRoot() const;
    std::string                 getIndex() const;
    std::string                 getClientMaxBody() const;

   private:
    // required server parameters
    int                         port;
    std::string                 interface;
    std::vector<LocationConfig> locations; // it least one location

    // optional server parameters
    std::string serverName;        // default: ""
    std::string root;              // default: use for location if not set(be required)
    std::string index;             // default: "index.html"
    std::string clientMaxBodySize; // default: "1M" or inherited from http config
};
#endif