#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/types.h>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include "../handlers/CgiProcess.hpp"
#include "../utils/Types.hpp"

class Client {
   private:
    int        client_fd;
    String     storeReceiveData;
    String     storeSendData;
    time_t     lastActivity;
    CgiProcess _cgi;

   public:
    Client(const Client&);
    Client& operator=(const Client&);
    Client(int fd);
    Client();
    ~Client();

    ssize_t receiveData();
    ssize_t sendData();
    void    setSendData(const String& data);
    void    clearStoreReceiveData();
    bool    isTimedOut(int timeout) const;
    void    closeConnection();
    String  getStoreReceiveData() const;
    String  getStoreSendData() const;
    int     getFd() const;

    CgiProcess&       getCgi();
    const CgiProcess& getCgi() const;
};

#endif
