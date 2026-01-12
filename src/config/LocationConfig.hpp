#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP
#include <iostream>

class LocationConfig {
    std::string path;
    std::string root;
    bool        autoindex;

   public:
    LocationConfig();
    LocationConfig(const std::string& p);

    void setRoot(const std::string& r);
    void setAutoIndex(bool v);

    std::string getPath() const;
    std::string getRoot() const;
    bool        getAutoIndex() const;
};

#endif