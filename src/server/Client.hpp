#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <unistd.h>
#include <string>

class Client {
   private:
    static const int INVALID_FD = -1;
    int              client_fd;
    std::string      buffer;

    Client(const Client&);
    Client& operator=(const Client&);

   public:
    Client();
    explicit Client(int fd);
    ~Client();

    ssize_t     receiveData();
    ssize_t     sendData(const std::string& data);
    std::string getBuffer() const;
    void        clearBuffer();
    void        closeConnection();
    int         getFd() const;
};

#endif
