
#include "HttpRequest.hpp"
/* Header Section */
// POST /upload/file.txt?user=loay&id=42 HTTP/1.1\r\n (request line)
// Host: localhost:8080\r\n (header lines)
// User-Agent: curl/7.88.1\r\n (header lines)
// Accept: */*\r\n (header lines)
// Content-Type: text/plain\r\n (header lines)
// Content-Length: 11\r\n (header lines)
// Connection: keep-alive\r\n\r\n (\r\n\r\n indicates end of headers)

/* Body Section */
// hello world

bool HttpRequest::parse(const std::string& raw) {
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;
    std::string headerSection = raw.substr(0, headerEnd);
    std::string bodySection   = raw.substr(headerEnd + 4); // +4 to skip \r\n\r\n
    if (!parseHeaders(headerSection))
        return false;
    if (!parseBody(bodySection))
        return false;
    return true;
}
bool HttpRequest::parseHeaders(const std::string& headerSection) {
    size_t lineEnd = headerSection.find("\r\n");
    if (lineEnd == std::string::npos)
        return false;

    std::string requestLine = headerSection.substr(0, lineEnd);

    std::stringstream ss(requestLine);
    ss >> method >> uri >> httpVersion;
    if (method.empty() || uri.empty() || httpVersion.empty())
        return false;

    method = toUpperWords(method);
    if (!checkAllowedMethods(method))
        return false;

    size_t q = uri.find('?');
    if (q != std::string::npos) {
        queryString = uri.substr(q + 1);
        uri         = uri.substr(0, q);
    }

    size_t pos = lineEnd + 2;
    while (pos < headerSection.size()) {
        lineEnd = headerSection.find("\r\n", pos);
        if (lineEnd == std::string::npos)
            break;

        std::string line = headerSection.substr(pos, lineEnd - pos);
        if (line.empty())
            break;

        std::string key, value;
        if (!splitByChar(line, key, value, ':'))
            return false;

        std::string headerKey = toLowerWords(trimSpacesComments(key));
        std::string headerVal = trimSpacesComments(value);

        headers[headerKey] = headerVal;

        if (headerKey == "content-length")
            std::stringstream(headerVal) >> contentLength;
        else if (headerKey == "content-type")
            contentType = headerVal;

        pos = lineEnd + 2;
    }
    return true;
}

bool HttpRequest::parseBody(const std::string& bodySection) {
    body = bodySection;
    if (headers.count("content-length")) {
        if (body.size() != contentLength)
            return false;
    } else {
        if (!body.empty())
            return false;
    }
    return true;
}

// Getters
std::string HttpRequest::getMethod() const {
    return method;
}
std::string HttpRequest::getUri() const {
    return uri;
}
std::string HttpRequest::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}
std::string HttpRequest::getBody() const {
    return body;
}
size_t HttpRequest::getContentLength() const {
    return contentLength;
}
std::string HttpRequest::getContentType() const {
    return contentType;
}
// Validators
bool HttpRequest::isComplete() const {
    return !method.empty() && !uri.empty() && !httpVersion.empty();
}
bool HttpRequest::hasBody() const {
    return !body.empty();
}