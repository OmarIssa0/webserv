#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP
#include <iostream>
#include <map>
#include <string>
class HttpRequest {
   private:
    std::string method;
    std::string uri;
    std::map<std::string, std::string> headers;
    std::string body;

   public:
    HttpRequest();
    ~HttpRequest();
    void        parseRequest(const std::string& rawRequest);
    void        print() const;
    std::string getUri() const;
};
#endif