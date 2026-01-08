#include <poll.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <signal.h>
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "server/Client.hpp"
#include "server/PollManager.hpp"
#include "server/Server.hpp"
#include "utils/Logger.hpp"
#include "utils/Utils.hpp"  // Include global utility

bool g_running = true;

void signalHandler(int signum) {
    (void)signum; 
    g_running = false;
}

std::string check_URI(const std::string& uri) {
    HttpResponse response;
    
    if (uri == "/") {
        response.setStatus(200, "OK");
        response.addHeader("Content-Type", "text/plain");
        response.setBody("Hello, World!");
    } else if (uri == "/about") {
        response.setStatus(200, "OK");
        response.addHeader("Content-Type", "text/plain");
        response.setBody("This is the about page.");
    } else {
        response.setStatus(404, "Not Found");
        response.addHeader("Content-Type", "text/plain");
        response.setBody("Not Found");
    }
    
    return response.toString();
}

int main(int argc, char** argv) {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    uint16_t    port = (argc > 1) ? std::atoi(argv[1]) : 9090;
    Server      server(port);
    if (!server.init()) {
        Logger::error("Failed to initialize server");
        return 1;
    }

    // Using toString with Logger - now you can log numbers easily!
    Logger::info("Server ready on port " + toString(port));

    std::vector<Client*> clients;
    PollManager          pollManager;
    pollManager.addFd(server.getFd(), POLLIN);

    while (g_running) {
        if (pollManager.pollConnections(100) < 0) 
            break;

        // New connection
        if (pollManager.hasEvent(0, POLLIN)) {
            int fd = server.acceptConnection();
            if (fd >= 0) {
                clients.push_back(new Client(fd));
                pollManager.addFd(fd, POLLIN);
                Logger::info("Client connected");
            }
        }

        for (size_t i = pollManager.size(); i > 1; i--) {
            size_t idx = i - 1;

            if (pollManager.hasEvent(idx, POLLIN)) {
                Client* client    = clients[idx - 1];
                ssize_t bytesRead = client->receiveData();

                if (bytesRead <= 0) {
                    Logger::info("Client disconnected");
                    client->closeConnection();
                    delete client;
                    clients.erase(clients.begin() + (idx - 1));
                    pollManager.removeFd(idx);
                } else {
                    HttpRequest request;
                    request.parseRequest(client->getBuffer());
                    request.print();

                    client->sendData(check_URI(request.getUri()));
                    client->clearBuffer();
                }
            } else if (pollManager.hasEvent(idx, POLLERR | POLLHUP)) {
                Client* client = clients[idx - 1];
                Logger::info("Client error/disconnect");
                client->closeConnection();
                delete client;
                clients.erase(clients.begin() + (idx - 1));
                pollManager.removeFd(idx);
            } else if (pollManager.hasEvent(idx, POLLNVAL)) {
                Client* client = clients[idx - 1];
                Logger::info("Client invalid request");
                client->closeConnection();
                delete client;
                clients.erase(clients.begin() + (idx - 1));
                pollManager.removeFd(idx);
            }
        }
    }
    Logger::info("Shutting down server...");
    for (size_t i = 0; i < clients.size(); i++) {
        clients[i]->closeConnection();
        delete clients[i];
    }
    server.stop();
    Logger::info("Server stopped");

    return 0;
}
