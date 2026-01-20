// #include <csignal>
// #include <cstring>
// #include <iostream>
// #include "config/ConfigParser.hpp"
// #include "server/ServerManager.hpp"
// #include "utils/Logger.hpp"

// ServerManager* g_serverManager = NULL;

// void signalHandler(int signum) {
//     if (signum == SIGINT || signum == SIGTERM) {
//         std::cout << "\nShutdown signal received..." << std::endl;
//         if (g_serverManager) {
//             g_serverManager->shutdown();
//         }
//     }
// }

// void setupSignals() {
//     struct sigaction sa;
//     std::memset(&sa, 0, sizeof(sa));
//     sa.sa_handler = signalHandler;
//     sigemptyset(&sa.sa_mask);
//     sa.sa_flags = 0;

//     sigaction(SIGINT, &sa, NULL);
//     sigaction(SIGTERM, &sa, NULL);
//     signal(SIGPIPE, SIG_IGN);
// }

// int main(int ac, char** av) {
//     std::string configFile = (ac > 1) ? av[1] : "webserv.conf";

//     std::cout << "========================================" << std::endl;
//     std::cout << "       Webserv HTTP Server v1.0        " << std::endl;
//     std::cout << "========================================\n" << std::endl;

//     ConfigParser parser(configFile);
//     if (!parser.parse()) {
//         return 1;
//     }

//     std::vector<ServerConfig> configs = parser.getServers();
//     if (configs.empty()) {
//         std::cout << "[ERROR]: No server configurations found" << std::endl;
//         return 1;
//     }

//     ServerManager serverManager;
//     g_serverManager = &serverManager;

//     if (!serverManager.initialize(configs)) {
//         std::cout << "[ERROR]: Failed to initialize server manager" << std::endl;
//         return 1;
//     }

//     std::cout << "\n========================================" << std::endl;
//     std::cout << "  Servers: " << serverManager.getServerCount() << std::endl;
//     std::cout << "  Status: Running" << std::endl;
//     std::cout << "========================================\n" << std::endl;

//     setupSignals();
//     serverManager.run();

//     std::cout << "\n========================================" << std::endl;
//     std::cout << "       Server Stopped Successfully      " << std::endl;
//     std::cout << "========================================" << std::endl;

//     return 0;
// }

#include <csignal>
#include <iostream>
#include "config/ConfigParser.hpp"

bool g_running = true;

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
    for (size_t i = 0; i < loc.getAllowedMethods().size(); i++) {
        std::cout << "    method     : " << loc.getAllowedMethods()[i] << "\n";
    }
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
    if (!parser.parse()) {
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
