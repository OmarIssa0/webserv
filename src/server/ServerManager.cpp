#include "ServerManager.hpp"

ServerManager::ServerManager()
    : running(false), pollManager(), servers(), serverConfigs(), clients(), clientToServer(), serverToConfigs(), mimeTypes() {}

ServerManager::ServerManager(const ServerManager& other)
    : running(other.running),
      pollManager(other.pollManager),
      servers(other.servers),
      serverConfigs(other.serverConfigs),
      clients(other.clients),
      clientToServer(other.clientToServer),
      serverToConfigs(other.serverToConfigs),
      mimeTypes(other.mimeTypes) {}

ServerManager& ServerManager::operator=(const ServerManager& other) {
    if (this != &other) {
        running         = other.running;
        pollManager     = other.pollManager;
        servers         = other.servers;
        clients         = other.clients;
        clientToServer  = other.clientToServer;
        serverToConfigs = other.serverToConfigs;
        mimeTypes       = other.mimeTypes;
    }
    return *this;
}

ServerManager::ServerManager(const VectorServerConfig& _configs)
    : running(false), pollManager(), servers(), serverConfigs(_configs), clients(), clientToServer(), serverToConfigs(), mimeTypes() {}

ServerManager::~ServerManager() {
    shutdown();
}

bool ServerManager::initialize() {
    if (serverConfigs.empty())
        return Logger::error("[ERROR]: No server configurations provided");
    if (!initializeServers(serverConfigs) || servers.empty())
        return Logger::error("[ERROR]: Failed to initialize servers");
    Logger::info("[INFO]: All servers initialized successfully");
    running = true;
    return Logger::info("[INFO]: ServerManager initialized");
}

ListenerToConfigsMap ServerManager::mapListenersToConfigs(const VectorServerConfig& configs) {
    ListenerToConfigsMap result;
    for (size_t i = 0; i < configs.size(); i++) {
        const VectorListenAddress& addresses = configs[i].getListenAddresses();
        for (size_t j = 0; j < addresses.size(); j++) {
            String key = addresses[j].getInterface() + ":" + typeToString<int>(addresses[j].getPort());
            result[key].push_back(configs[i]);
        }
    }
    return result;
}

Server* ServerManager::createServerForListener(const String& listenerKey, const VectorServerConfig& configsForListener, PollManager& pollManager) {
    if (configsForListener.empty())
        return NULL;

    const ServerConfig& firstConfig = configsForListener[0];

    const VectorListenAddress& addresses   = firstConfig.getListenAddresses();
    size_t                     listenIndex = 0;
    for (size_t i = 0; i < addresses.size(); i++) {
        String key = addresses[i].getInterface() + ":" + typeToString<int>(addresses[i].getPort());
        if (key == listenerKey) {
            listenIndex = i;
            break;
        }
    }
    Server* server = NULL;
    try {
        server = new Server(firstConfig, listenIndex);
        if (!server->init())
            throw std::runtime_error("Server init failed");
    } catch (...) {
        Logger::error("[ERROR]: Failed to create server for listener " + listenerKey);
        if (server)
            delete server;
        return NULL;
    }

    pollManager.addFd(server->getFd(), POLLIN);
    return server;
}

bool ServerManager::initializeServers(const VectorServerConfig& configs) {
    ListenerToConfigsMap listenerToConfigs = mapListenersToConfigs(configs);

    std::map<String, Server*> listenerToServerMap;

    ListenerToConfigsMap::iterator it;
    for (it = listenerToConfigs.begin(); it != listenerToConfigs.end(); ++it) {
        const String&             listenerKey        = it->first;
        const VectorServerConfig& configsForListener = it->second;

        Server* server = createServerForListener(listenerKey, configsForListener, pollManager);
        if (!server)
            continue;

        servers.push_back(server);
        listenerToServerMap[listenerKey] = server;
        serverToConfigs[server->getFd()] = configsForListener;
    }

    return !servers.empty();
}

bool ServerManager::run() {
    if (!running)
        return Logger::error("[ERROR]: Cannot run server manager");

    while (running) {
        int eventCount = pollManager.pollConnections(100);
        checkTimeouts(CLIENT_TIMEOUT);
        if (eventCount <= 0)
            continue;

        for (size_t i = 0; i < pollManager.size() && eventCount > 0; i++) {
            int  fd     = pollManager.getFd(i);
            bool hasIn  = pollManager.hasEvent(i, POLLIN); // when client sends data or new connection on server socket
            bool hasOut = pollManager.hasEvent(
                i, POLLOUT); // when client socket is ready to send data (after response is set) or when CGI pipe is ready to write
            bool hasHup = pollManager.hasEvent(i, POLLHUP);
            bool hasErr = pollManager.hasEvent(i, POLLERR);

            if (!hasIn && !hasOut && !hasHup && !hasErr)
                continue;
            // CGI pipe events
            if (isCgiPipe(fd)) {
                if (hasOut)
                    handleCgiWrite(fd);
                if (hasIn || hasHup || hasErr)
                    handleCgiRead(fd);
                eventCount--;
                continue;
            }
            // server or client socket events
            if (hasIn) {
                // server socket event (new connection)
                if (isServerSocket(fd)) {
                    Server* server = findServerByFd(fd);
                    if (server)
                        acceptNewConnection(server);
                    // client socket event (data received)
                } else if (clients.find(fd) != clients.end()) {
                    handleClientRead(fd);
                }
                eventCount--;
            }
            // client socket ready to send data
            if (hasOut) {
                if (clients.find(fd) != clients.end()) {
                    handleClientWrite(fd);
                }
                eventCount--;
            }
        }
    }
    return true;
}

bool ServerManager::acceptNewConnection(Server* server) {
    int clientFd = server->acceptConnection();
    if (clientFd < 0)
        return Logger::error("[ERROR]: Failed to accept new connection");

    Client* client = NULL;
    try {
        client = new Client(clientFd);
    } catch (const std::bad_alloc& e) {
        Logger::error("[ERROR]: Memory allocation failed for client");
        close(clientFd);
        return false;
    }
    clients[clientFd]        = client;
    clientToServer[clientFd] = server;
    pollManager.addFd(clientFd, POLLIN | POLLOUT);
    return Logger::info("[INFO]: Connection accepted on port " + typeToString(server->getPort()));
}

void ServerManager::handleClientRead(int clientFd) {
    Client* client = getValue(clients, clientFd, (Client*)NULL);
    if (client == NULL) {
        closeClientConnection(clientFd);
        return;
    }

    if (client->receiveData() <= 0) {
        closeClientConnection(clientFd);
        return;
    }
    // dont process request until the cgi done sending response for client to avoid mixing the response of cgi and the new request if client sent another request before reading the cgi response
    if (client->getCgi().isActive())
        return;
    Server* server = getValue(clientToServer, clientFd, (Server*)NULL);
    if (server)
        processRequest(client, server);
}

void ServerManager::handleClientWrite(int clientFd) {
    Client* client = getValue(clients, clientFd, (Client*)NULL);
    if (client == NULL)
        return;

    if (client->getStoreSendData().empty())
        return;

    ssize_t sent = client->sendData();
    if (sent < 0) {
        closeClientConnection(clientFd);
        return;
    }

    // If all data sent, close connection
    if (client->getStoreSendData().empty()) {
        closeClientConnection(clientFd);
    }
}

void ServerManager::checkTimeouts(int timeout) {
    std::vector<int> toClose;

    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        // timeout for CGI processes it have running for time if end time exceed the CGI_TIMEOUT
        if (it->second->getCgi().isActive()) {
            if (getDifferentTime(it->second->getCgi().getStartTime(), getCurrentTime()) > CGI_TIMEOUT) {
                Logger::info("[INFO]: CGI timeout, killing process");
                cleanupClientCgi(it->second);
                ResponseBuilder builder(mimeTypes);
                HttpResponse    response = builder.buildError(HTTP_GATEWAY_TIMEOUT, "CGI Timeout");
                it->second->setSendData(response.toString());
            }
            // timeout for client connections
        } else if (it->second->isTimedOut(timeout)) {
            toClose.push_back(it->first);
        }
    }

    for (size_t i = 0; i < toClose.size(); i++) {
        Logger::info("[INFO]: Client timeout, closing connection");
        closeClientConnection(toClose[i]);
    }
}


String checkMethod(std::string& buffer) {
    size_t pos       = buffer.find("\r\n");
    String firstLine = buffer.substr(0, pos);
    if (firstLine.find("POST") == 0)
        return "POST";
    if (firstLine.find("GET") == 0)
        return "GET";
    if (firstLine.find("DELETE") == 0)
        return "DELETE";
    if (firstLine.find("PUT") == 0)
        return "PUT";
    return "";
}

void ServerManager::processRequest(Client* client, Server* server) {
    String buffer = client->getStoreReceiveData();
    if (buffer.size() > MAX_HEADER_SIZE) {
        ResponseBuilder builder;
        HttpResponse    response = builder.buildError(HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE, "Request Header Fields Too Large");
        client->setSendData(response.toString());
        client->clearStoreReceiveData();
        return;
    }
    size_t headerEnd    = buffer.find("\r\n\r\n");
    size_t headerEndLen = 4;
    if (headerEnd == String::npos) {
        headerEnd    = buffer.find("\n\n");
        headerEndLen = 2;
    }
    if (headerEnd == String::npos)
        return;

    String headerSection = buffer.substr(0, headerEnd);
    bool   isChunked     = isChunkedTransferEncoding(headerSection);

    String fullRequest;
    if (isChunked) {
        // For chunked encoding, look for the terminator "0\r\n\r\n"
        String bodyPart   = buffer.substr(headerEnd + headerEndLen);
        size_t terminator = bodyPart.find("0\r\n\r\n");
        if (terminator == String::npos)
            return; // not all chunks received yet

        String chunkedBody = bodyPart.substr(0, terminator + 5); // include "0\r\n\r\n"
        String decodedBody;
        if (!decodeChunkedBody(chunkedBody, decodedBody)) {
            ResponseBuilder builder;
            HttpResponse    response = builder.buildError(HTTP_BAD_REQUEST, "Invalid chunked encoding");
            client->setSendData(response.toString());
            client->clearStoreReceiveData();
            return;
        }

        size_t maxBodySize = convertMaxBodySize(clientToServer[client->getFd()]->getConfig().getClientMaxBody());
        if (decodedBody.size() > maxBodySize) {
            ResponseBuilder builder;
            HttpResponse    response = builder.buildError(HTTP_PAYLOAD_TOO_LARGE, "Payload Too Large");
            client->setSendData(response.toString());
            client->clearStoreReceiveData();
            return;
        }

        // Rebuild request: original headers (minus Transfer-Encoding) + Content-Length + decoded body
        fullRequest = headerSection + "\r\nContent-Length: " + typeToString<size_t>(decodedBody.size()) + "\r\n\r\n" + decodedBody;
    } else {
        size_t contentLength = extractContentLength(headerSection);
        size_t fullSize      = headerEnd + headerEndLen + contentLength;
        size_t maxBodySize   = convertMaxBodySize(clientToServer[client->getFd()]->getConfig().getClientMaxBody());
        if (contentLength > maxBodySize) {
            ResponseBuilder builder;
            HttpResponse    response = builder.buildError(HTTP_PAYLOAD_TOO_LARGE, "Payload Too Large");
            client->setSendData(response.toString());
            client->clearStoreReceiveData();
            return;
        }
        if (buffer.size() < fullSize)
            return;

        fullRequest = buffer.substr(0, fullSize);
    }

    HttpRequest request;
    if (!request.parse(fullRequest)) {
        ResponseBuilder builder;
        HttpResponse    response = builder.buildError(HTTP_BAD_REQUEST, "Bad Request");
        client->setSendData(response.toString());
        client->clearStoreReceiveData();
        return;
    }

    Router      router(serverToConfigs[server->getFd()], request);
    RouteResult result = router.processRequest();

    ResponseBuilder builder(mimeTypes);
    // the cgi to handle cgi if is not cgi it send it as null value
    HttpResponse response = builder.build(result, &client->getCgi());
    if (client->getCgi().isActive()) {
        registerCgiPipes(client);
        client->clearStoreReceiveData();
        return;
    }
    client->setSendData(response.toString());
    client->clearStoreReceiveData();
}

void ServerManager::closeClientConnection(int clientFd) {
    Client* c = getValue(clients, clientFd, (Client*)NULL);
    if (c && c->getCgi().isActive())
        cleanupClientCgi(c);

    for (size_t i = 0; i < pollManager.size(); i++) {
        if (pollManager.getFd(i) == clientFd) {
            pollManager.removeFd(i);
            break;
        }
    }
    if (c) {
        c->closeConnection();
        delete c;
    }
    clients.erase(clientFd);
    clientToServer.erase(clientFd);
}

bool ServerManager::isCgiPipe(int fd) const {
    return cgiPipeToClient.find(fd) != cgiPipeToClient.end();
}

void ServerManager::removeCgiPipe(int pipeFd) {
    for (size_t i = 0; i < pollManager.size(); i++) {
        if (pollManager.getFd(i) == pipeFd) {
            pollManager.removeFd(i);
            break;
        }
    }
    cgiPipeToClient.erase(pipeFd);
}

void ServerManager::registerCgiPipes(Client* client) {
    CgiProcess& cgi      = client->getCgi();
    int         clientFd = client->getFd();
    if (!cgi.isWriteDone()) {
        pollManager.addFd(cgi.getWriteFd(), POLLOUT);
        cgiPipeToClient[cgi.getWriteFd()] = clientFd;
    } else {
        close(cgi.getWriteFd());
        cgi.setWriteFd(-1);
    }
    pollManager.addFd(cgi.getReadFd(), POLLIN);
    cgiPipeToClient[cgi.getReadFd()] = clientFd;
    Logger::info("[INFO]: CGI process started (pid " + typeToString<int>(cgi.getPid()) + ")");
}

void ServerManager::handleCgiWrite(int pipeFd) {
    std::map<int, int>::iterator it = cgiPipeToClient.find(pipeFd);
    if (it == cgiPipeToClient.end())
        return;
    Client* client = getValue(clients, it->second, (Client*)NULL);
    if (!client || !client->getCgi().isActive()) {
        removeCgiPipe(pipeFd);
        return;
    }
    if (client->getCgi().writeBody(pipeFd)) {
        removeCgiPipe(pipeFd);
        close(pipeFd);
        client->getCgi().setWriteFd(-1);
    }
}

void ServerManager::handleCgiRead(int pipeFd) {
    std::map<int, int>::iterator it = cgiPipeToClient.find(pipeFd);
    if (it == cgiPipeToClient.end())
        return;
    Client* client = getValue(clients, it->second, (Client*)NULL);
    if (!client || !client->getCgi().isActive()) {
        removeCgiPipe(pipeFd);
        return;
    }
    if (!client->getCgi().handleRead()) {
        removeCgiPipe(pipeFd);
        if (client->getCgi().getWriteFd() != -1)
            removeCgiPipe(client->getCgi().getWriteFd());
        ResponseBuilder builder(mimeTypes);
        client->setSendData(builder.buildCgiResponse(client->getCgi()).toString());
        Logger::info("[INFO]: CGI process finished");
    }
}

void ServerManager::cleanupClientCgi(Client* client) {
    if (!client->getCgi().isActive())
        return;
    if (client->getCgi().getWriteFd() != -1)
        removeCgiPipe(client->getCgi().getWriteFd());
    if (client->getCgi().getReadFd() != -1)
        removeCgiPipe(client->getCgi().getReadFd());
    client->getCgi().cleanup();
}

// ─── Server helpers ──────────────────────────────────────────────────────────

Server* ServerManager::findServerByFd(int serverFd) const {
    for (size_t i = 0; i < servers.size(); i++) {
        if (servers[i]->getFd() == serverFd) {
            return servers[i];
        }
    }
    return NULL;
}

bool ServerManager::isServerSocket(int fd) const {
    for (size_t i = 0; i < servers.size(); i++)
        if (servers[i]->getFd() == fd)
            return true;
    return false;
}

void ServerManager::shutdown() {
    if (!running)
        return;

    std::cout << "[INFO]: Shutting down..." << std::endl;
    running = false;

    for (MapIntClientPtr::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getCgi().isActive())
            cleanupClientCgi(it->second);
        it->second->closeConnection();
        delete it->second;
    }
    clients.clear();
    clientToServer.clear();
    cgiPipeToClient.clear();

    for (size_t i = 0; i < servers.size(); i++) {
        servers[i]->stop();
        delete servers[i];
    }
    servers.clear();
    std::cout << "[INFO]: Shutdown complete" << std::endl;
}

size_t ServerManager::getServerCount() const {
    return servers.size();
}

size_t ServerManager::getClientCount() const {
    return clients.size();
}
