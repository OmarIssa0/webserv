#include "HttpRequest.hpp"
HttpRequest::HttpRequest() {}
HttpRequest::~HttpRequest() {}

void HttpRequest::parseRequest(const std::string& rawRequest) {
    size_t pos = 0;
    size_t lineEnd = rawRequest.find("\r\n");
    if (lineEnd == std::string::npos) return;

    std::string requestLine = rawRequest.substr(0, lineEnd);
    size_t methodEnd = requestLine.find(' ');
    if (methodEnd == std::string::npos) return;
    method = requestLine.substr(0, methodEnd);

    size_t uriEnd = requestLine.find(' ', methodEnd + 1);
    if (uriEnd == std::string::npos) return;
    uri = requestLine.substr(methodEnd + 1, uriEnd - methodEnd - 1);
    pos = lineEnd + 2;
    while ((lineEnd = rawRequest.find("\r\n", pos)) != std::string::npos) {
        std::string headerLine = rawRequest.substr(pos, lineEnd - pos);
        if (headerLine.empty()) {
            pos = lineEnd + 2;
            break;
        }
        size_t colonPos = headerLine.find(':');
        if (colonPos != std::string::npos) {
            std::string key = headerLine.substr(0, colonPos);
            std::string value = headerLine.substr(colonPos + 1);
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                value.erase(0, 1);
            }
            headers[key] = value;
        }
        pos = lineEnd + 2;
    }
    body = rawRequest.substr(pos);
}

std::string HttpRequest::getUri() const {
    return uri;
}

void HttpRequest::print() const {
    std::cout << "Method: " << method << std::endl;
    std::cout << "URI: " << uri << std::endl;
    std::cout << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
         it != headers.end(); ++it) {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    std::cout << "Body: " << body << std::endl;
}