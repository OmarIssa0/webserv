#ifndef SERVER_HPP
#define SERVER_HPP

#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <iostream>
class Server {
   private:
    static const int INVALID_FD = -1;
    int              server_fd;
    uint16_t         port;
    bool             running;

    // steps to initialize server
    bool createSocket();
    bool configureSocket();
    bool bindSocket();
    bool startListening();
    bool createNonBlockingSocket(int fd);

    // force non-copyable for copy constructor and assignment operator
    Server(const Server&);
    Server& operator=(const Server&);

   public:
    Server();
    explicit Server(uint16_t port);
    ~Server();

    bool init();
    void stop();
    int  acceptConnection(sockaddr_in* client_addr = 0);

    // getters
    int getFd() const;
};

#endif
