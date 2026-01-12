#include "MimeTypes.hpp"
MimeTypes::MimeTypes(const std::string& extension) : extension(extension) {
    if (extension == "html" || extension == "htm")
        mimeType = "text/html";
    else if (extension == "css")
        mimeType = "text/css";
    else if (extension == "js")
        mimeType = "application/javascript";
    else if (extension == "png")
        mimeType = "image/png";
    else if (extension == "jpg" || extension == "jpeg")
        mimeType = "image/jpeg";
    else if (extension == "gif")
        mimeType = "image/gif";
    else if (extension == "txt")
        mimeType = "text/plain";
    else
        mimeType = "application/octet-stream";
}

MimeTypes::~MimeTypes() {}

std::string MimeTypes::getMimeType() const {
    return mimeType;
}