#include <iostream>
#include "config/ConfigParser.hpp"

/* ----------------------------------------------------
 * Helper printer
 * ---------------------------------------------------- */
void printLine() {
    std::cout << "----------------------------------------\n";
}

/* ----------------------------------------------------
 * Print location
 * ---------------------------------------------------- */
void printLocation(const LocationConfig& loc, const std::string& resolvedBody) {
    std::cout << "  Location: " << loc.getPath() << "\n";
    std::cout << "    root       : " << loc.getRoot() << "\n";
    std::cout << "    autoindex  : " << (loc.getAutoIndex() ? "on" : "off") << "\n";

    if (loc.getClientMaxBody().empty() == false)
        std::cout << "    client_max : " << loc.getClientMaxBody() << " (location)\n";
    else
        std::cout << "    client_max : " << resolvedBody << " (fallback)\n";
}

/* ----------------------------------------------------
 * Print server
 * ---------------------------------------------------- */
void printServer(const ServerConfig& srv, const ConfigParser& http) {
    printLine();
    std::cout << "Server\n";
    std::cout << "  listen       : " << srv.getPort() << "\n";
    std::cout << "  server_name  : " << srv.getServerName() << "\n";
    std::cout << "  root         : " << srv.getRoot() << "\n";

    if (srv.getClientMaxBody().empty() == false)
        std::cout << "  client_max   : " << srv.getClientMaxBody() << " (server)\n";
    else
        std::cout << "  client_max   : " << http.getHttpClientMaxBody() << " (http fallback)\n";

    const std::vector<LocationConfig>& locs = srv.getLocations();

    for (size_t i = 0; i < locs.size(); i++) {
        std::string resolved = http.getHttpClientMaxBody();

        if (srv.getClientMaxBody().empty() == false)
            resolved = srv.getClientMaxBody();

        if (locs[i].getClientMaxBody().empty() == false)
            resolved = locs[i].getClientMaxBody();

        printLocation(locs[i], resolved);
    }
}

/* ----------------------------------------------------
 * MAIN TESTER
 * ---------------------------------------------------- */
int main(int ac, char** av) {
    if (ac != 2) {
        std::cerr << "Usage: ./webserv <config_file>\n";
        return 1;
    }

    ConfigParser parser(av[1]);

    std::cout << "\nLoading config: " << av[1] << "\n";

    if (!parser.parse()) {
        std::cerr << "ERROR: " << parser.getError() << "\n";
        return 1;
    }

    const std::vector<ServerConfig>& servers = parser.getServers();

    /* ------------------------------------------------
     * Global HTTP
     * ------------------------------------------------ */
    printLine();
    std::cout << "HTTP\n";
    std::cout << "  client_max_body_size : " << parser.getHttpClientMaxBody() << "\n";

    /* ------------------------------------------------
     * Servers
     * ------------------------------------------------ */
    if (servers.empty()) {
        std::cerr << "❌ ERROR: No server block found\n";
        return 1;
    }

    for (size_t i = 0; i < servers.size(); i++) {
        printServer(servers[i], parser);
    }

    printLine();
    std::cout << "✅ CONFIG OK\n";
    return 0;
}

// #include <poll.h>
// #include <signal.h>
// #include <cstdio>
// #include <cstdlib>
// #include <filesystem>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include <vector>
// #include "config/ConfigParser.hpp"
// // #include "http/HttpRequest.hpp"
// // #include "http/HttpResponse.hpp"
// // #include "server/Client.hpp"
// // #include "server/PollManager.hpp"
// // #include "server/Server.hpp"
// // #include "utils/Logger.hpp"
// // #include "utils/Utils.hpp"
// // #include "config/MimeTypes.hpp"

// // bool g_running = true;

// // void signalHandler(int signum) {
// //     (void)signum;
// //     g_running = false;
// // }

// // std::string check_URI(const std::string& uri) {
// //     HttpResponse response;

// //     if (uri == "/") {
// //         response.setStatus(200, "OK");
// //         response.addHeader("Content-Type", "text/plain");
// //         response.setBody("Hello, World!");
// //     } else if (uri == "/about") {
// //         response.setStatus(200, "OK");
// //         response.addHeader("Content-Type", "text/plain");
// //         response.setBody("This is the about page.");
// //     } else {
// //         response.setStatus(404, "Not Found");
// //         response.addHeader("Content-Type", "text/plain");
// //         response.setBody("Not Found");
// //     }

// //     return response.toString();
// // }

// void printConfig(const ConfigParser& parser) {
//     std::vector<ServerConfig> servers = parser.getServers();
//     std::cout << "Parsed " << servers.size() << " server(s) from configuration." << std::endl;
//     for (size_t i = 0; i < servers.size(); ++i) {
//         const ServerConfig& srv = servers[i];
//         std::cout << "==================== Server " << i + 1 << " ====================" << std::endl;
//         std::cout << "Port        : " << srv.getPort() << std::endl;
//         std::string sname = srv.getServerName();
//         std::cout << "Server Name : " << (sname.empty() ? "<none>" : sname) << std::endl;
//         std::string sroot = srv.getRoot();
//         std::cout << "Root        : " << (sroot.empty() ? "<none>" : sroot) << std::endl;

//         std::vector<LocationConfig> locations = srv.getLocations();
//         std::cout << "Locations   : " << locations.size() << std::endl;
//         for (size_t j = 0; j < locations.size(); ++j) {
//             const LocationConfig& loc = locations[j];
//             std::cout << "  ----------------------------------------" << std::endl;
//             std::cout << "  Location " << j + 1 << std::endl;
//             std::string lpath = loc.getPath();
//             std::string lroot = loc.getRoot();
//             std::cout << "    Path     : " << (lpath.empty() ? "<none>" : lpath) << std::endl;
//             std::cout << "    Root     : " << (lroot.empty() ? "<none>" : lroot) << std::endl;
//             std::cout << "    AutoIndex: " << (loc.getAutoIndex() ? "on" : "off") << std::endl;
//         }
//         std::cout << std::endl;
//     }
// }

// int main(int argc, char** argv) {
//     (void)argc;
//     (void)argv;
//     std::cout << "Current path: " << std::filesystem::current_path() << std::endl;
//     std::fstream configFile("/mnt/c/Users/loay1/Desktop/42Amman/WebServ/webserv.conf");

//     if (!configFile.is_open()) {
//         std::cerr << "Failed to open configuration file." << std::endl;
//         return 1;
//     }
//     ConfigParser parser("/mnt/c/Users/loay1/Desktop/42Amman/WebServ/webserv.conf");
//     if (!parser.parse()) {
//         std::cerr << "Error parsing configuration: " << parser.getError() << std::endl;
//         return 1;
//     }
//     // print parsed server configurations
//     printConfig(parser);
//     // signal(SIGINT, signalHandler);
//     // signal(SIGTERM, signalHandler);

//     // uint16_t    port = (argc > 1) ? std::atoi(argv[1]) : 9090;
//     // Server      server(port);
//     // if (!server.init()) {
//     //     Logger::error("Failed to initialize server");
//     //     return 1;
//     // }

//     // Logger::info("Server ready on port " + toString(port));

//     // std::vector<Client*> clients;
//     // PollManager          pollManager;
//     // pollManager.addFd(server.getFd(), POLLIN);

//     // while (g_running) {
//     //     if (pollManager.pollConnections(100) < 0)
//     //         break;

//     //     if (pollManager.hasEvent(0, POLLIN)) {
//     //         int fd = server.acceptConnection();
//     //         if (fd >= 0) {
//     //             clients.push_back(new Client(fd));
//     //             pollManager.addFd(fd, POLLIN);
//     //             Logger::info("Client connected");
//     //         }
//     //     }

//     //     for (size_t i = pollManager.size(); i > 1; i--) {
//     //         size_t idx = i - 1;

//     //         if (pollManager.hasEvent(idx, POLLIN)) {
//     //             Client* client    = clients[idx - 1];
//     //             ssize_t bytesRead = client->receiveData();

//     //             if (bytesRead <= 0) {
//     //                 Logger::info("Client disconnected");
//     //                 client->closeConnection();
//     //                 delete client;
//     //                 clients.erase(clients.begin() + (idx - 1));
//     //                 pollManager.removeFd(idx);
//     //             } else {
//     //                 HttpRequest request;
//     //                 request.parseRequest(client->getBuffer());
//     //                 request.print();

//     //                 client->sendData(check_URI(request.getUri()));
//     //                 client->clearBuffer();
//     //             }
//     //         } else if (pollManager.hasEvent(idx, POLLERR | POLLHUP)) {
//     //             Client* client = clients[idx - 1];
//     //             Logger::info("Client error/disconnect");
//     //             client->closeConnection();
//     //             delete client;
//     //             clients.erase(clients.begin() + (idx - 1));
//     //             pollManager.removeFd(idx);
//     //         } else if (pollManager.hasEvent(idx, POLLNVAL)) {
//     //             Client* client = clients[idx - 1];
//     //             Logger::info("Client invalid request");
//     //             client->closeConnection();
//     //             delete client;
//     //             clients.erase(clients.begin() + (idx - 1));
//     //             pollManager.removeFd(idx);
//     //         }
//     //     }
//     // }
//     // Logger::info("Shutting down server...");
//     // for (size_t i = 0; i < clients.size(); i++) {
//     //     clients[i]->closeConnection();
//     //     delete clients[i];
//     // }
//     // server.stop();
//     // Logger::info("Server stopped");

//     return 0;
// }
