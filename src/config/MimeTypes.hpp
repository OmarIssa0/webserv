#ifndef MIME_TYPES_HPP
#define MIME_TYPES_HPP
#include <iostream>
class MimeTypes {
   public:
    MimeTypes(const std::string& extension);
    ~MimeTypes();
    std::string getMimeType() const;

   private:
    std::string extension;
    std::string mimeType;
};
#endif
