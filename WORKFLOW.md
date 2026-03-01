# Webserv - Complete Project Workflow & Architecture Guide

## Table of Contents
1. [Project Overview](#1-project-overview)
2. [Subject Compliance Audit](#2-subject-compliance-audit)
3. [Architecture Overview](#3-architecture-overview)
4. [Startup Flow](#4-startup-flow)
5. [Event Loop - The Heart of the Server](#5-event-loop---the-heart-of-the-server)
6. [Request Lifecycle](#6-request-lifecycle)
7. [Configuration Parsing](#7-configuration-parsing)
8. [Routing](#8-routing)
9. [Response Building](#9-response-building)
10. [CGI Handling](#10-cgi-handling)
11. [Session Management (Bonus)](#11-session-management-bonus)
12. [Clean Code Report](#12-clean-code-report)
13. [Performance Analysis](#13-performance-analysis)
14. [File-by-File Walkthrough](#14-file-by-file-walkthrough)

---

## 1. Project Overview

Webserv is a non-blocking HTTP/1.1 server written in C++98. It uses `poll()` as the single I/O multiplexing mechanism to handle multiple clients concurrently on a single thread. The server supports:

- **GET, POST, DELETE** methods (mandatory)
- **Static file serving** with MIME type detection
- **Directory listing** (autoindex)
- **File uploads** via multipart/form-data
- **CGI execution** (Python scripts via `.py` extension)
- **HTTP redirects** (`return 301 /target`)
- **Custom error pages**
- **Multiple virtual servers** on different ports
- **Keep-alive connections**
- **Chunked transfer encoding**
- **Cookie/session management** (bonus)

---

## 2. Subject Compliance Audit

### Allowed External Functions Used
| Function | Where Used | Status |
|----------|-----------|--------|
| `socket` | Server.cpp - creates TCP socket | OK |
| `bind` | Server.cpp - binds to address:port | OK |
| `listen` | Server.cpp - starts listening | OK |
| `accept` | Server.cpp - accepts connections | OK |
| `poll` | PollManager.cpp - I/O multiplexing | OK |
| `read` | Client.cpp, CgiProcess.cpp | OK |
| `write` | Client.cpp, CgiProcess.cpp | OK |
| `close` | Everywhere for FD cleanup | OK |
| `setsockopt` | Server.cpp - SO_REUSEADDR/PORT | OK |
| `getaddrinfo`/`freeaddrinfo` | Server.cpp - DNS resolution | OK |
| `fcntl` | Utils.cpp - O_NONBLOCK only | OK |
| `pipe` | CgiHandler.cpp - parent-child IPC | OK |
| `fork` | CgiHandler.cpp - CGI only | OK |
| `execve` | CgiHandler.cpp - run CGI interpreter | OK |
| `dup2` | CgiHandler.cpp - redirect stdin/stdout | OK |
| `waitpid` | CgiProcess.cpp - reap child | OK |
| `kill` | CgiProcess.cpp - terminate stuck CGI | OK |
| `signal` | main.cpp - SIGINT/SIGTERM/SIGPIPE | OK |
| `stat` | Utils.cpp - file existence/type | OK |
| `open` | Utils.cpp - /dev/urandom for GUID | OK |
| `opendir`/`readdir`/`closedir` | DirectoryListingHandler.cpp | OK |
| `chdir` | CgiHandler.cpp - set CGI working dir | OK |
| `access` | Not used but allowed | OK |

### Potential Concern
| Function | Where Used | Notes |
|----------|-----------|-------|
| `mkdir` | Utils.cpp `ensureDirectoryExists()` | NOT in allowed list. Used to auto-create upload directories. Could be flagged in evaluation. **Mitigation**: Ensure upload directories exist in advance, or accept the risk since it's a reasonable filesystem operation for uploads. |
| `std::remove` | DeleteHandler.cpp | C++ standard library function (not POSIX syscall), should be OK |

### Rule Compliance
| Rule | Status | Details |
|------|--------|---------|
| No crash under any circumstance | PASS | try/catch in main loop, signal handlers, NULL checks everywhere |
| Non-blocking at all times | PASS | All sockets set O_NONBLOCK via `fcntl(fd, F_SETFL, O_NONBLOCK)` |
| Single poll() for all I/O | PASS | One `PollManager::pollConnections()` call per iteration |
| No read/write without poll | PASS | Socket I/O only happens after poll reports readiness (POLLIN/POLLOUT) |
| No errno check after read/write | PASS | Code avoids errno; uses return values only |
| fork only for CGI | PASS | `fork()` only in `CgiHandler::handle()` |
| Makefile rules | PASS | Has `$(NAME)`, `all`, `clean`, `fclean`, `re` |
| C++98 compliance | PASS | Compiles with `-std=c++98 -Wall -Wextra -Werror` |
| Requests never hang | PASS | CLIENT_TIMEOUT (160s) and CGI_TIMEOUT (160s) enforced |
| Default error pages | PASS | `ErrorPageHandler::generateHtml()` generates animated HTML pages |

---

## 3. Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                        main.cpp                         │
│  Parse config → Initialize ServerManager → Run loop     │
└────────────────────────┬────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────┐
│                   ServerManager                         │
│  The "god class" - orchestrates everything              │
│  ┌─────────────┐ ┌──────────┐ ┌───────────────┐       │
│  │ PollManager  │ │ Server[] │ │ Client map    │       │
│  │ (poll loop)  │ │ (listen) │ │ (connections) │       │
│  └─────────────┘ └──────────┘ └───────────────┘       │
│  ┌─────────────┐ ┌──────────────┐ ┌──────────────┐    │
│  │ Router      │ │ResponseBuild.│ │SessionManager│    │
│  │ (matching)  │ │ (dispatch)   │ │ (bonus)      │    │
│  └─────────────┘ └──────────────┘ └──────────────┘    │
└─────────────────────────────────────────────────────────┘
                         │
          ┌──────────────┼──────────────┐
          ▼              ▼              ▼
   ┌────────────┐ ┌────────────┐ ┌────────────┐
   │  Config    │ │  HTTP      │ │  Handlers  │
   │  Layer     │ │  Layer     │ │  Layer     │
   │            │ │            │ │            │
   │ ConfigLexer│ │ HttpRequest│ │ Static     │
   │ ConfigPars.│ │ HttpResp.  │ │ CGI        │
   │ ServerConf.│ │ RouteResult│ │ Upload     │
   │ LocationCf.│ │ Router     │ │ Delete     │
   │ MimeTypes  │ │ RespBuild. │ │ DirListing │
   │ ListenAddr.│ │            │ │ ErrorPage  │
   └────────────┘ └────────────┘ └────────────┘
```

---

## 4. Startup Flow

### Step 1: `main.cpp` - Entry Point
```
main(ac, av)
  │
  ├─ 1. Determine config file (arg or "default.conf")
  ├─ 2. Verify config file and mime.types exist
  ├─ 3. Create ConfigParser and parse config
  ├─ 4. Extract VectorServerConfig from parser
  ├─ 5. Create ServerManager with configs
  ├─ 6. Setup signal handlers (SIGINT, SIGTERM, SIGPIPE)
  ├─ 7. Call serverManager.initialize()
  └─ 8. Call serverManager.run() — enters event loop
```

**Key detail**: `g_running` is a `volatile sig_atomic_t` global. It starts at `0`, is set to `1` in `initialize()`, and the signal handler sets it back to `0` to cleanly stop the server.

### Step 2: `ServerManager::initialize()`
```
initialize()
  │
  ├─ Validate configs exist
  ├─ Call initializeServers(configs)
  │   │
  │   ├─ mapListenersToConfigs() — groups server configs by "interface:port"
  │   │   Example: Two servers on 0.0.0.0:8080 → same listener, different server_names
  │   │
  │   └─ For each unique listener:
  │       ├─ createServerForListener() — creates Server object
  │       │   ├─ new Server(config, listenIndex)
  │       │   └─ server->init()
  │       │       ├─ createSocket()      → socket(AF_INET, SOCK_STREAM, 0)
  │       │       ├─ configureSocket()   → setsockopt(SO_REUSEADDR, SO_REUSEPORT)
  │       │       ├─ bindSocket()        → getaddrinfo() + bind()
  │       │       ├─ startListening()    → listen(fd, SOMAXCONN)
  │       │       └─ setNonBlocking(fd)  → fcntl(fd, F_SETFL, O_NONBLOCK)
  │       │
  │       ├─ pollManager.addFd(server_fd, POLLIN) — monitor for new connections
  │       ├─ Store server in servers[]
  │       ├─ Map server_fd → VectorServerConfig (for virtual hosting)
  │       └─ Map server_fd → Server* (for O(1) lookup)
  │
  └─ Set g_running = 1
```

**Why multiple configs per listener?** The subject says the server should listen on multiple ports for different content. Two `server {}` blocks can share the same `listen` directive — the Router uses `server_name` to distinguish them (virtual hosting).

---

## 5. Event Loop - The Heart of the Server

`ServerManager::run()` is the main event loop. It runs a single `poll()` call per iteration:

```
while (g_running):
  │
  ├── 1. poll(fds, timeout=100ms)
  │       Returns how many FDs had events
  │
  ├── 2. checkTimeouts(CLIENT_TIMEOUT)
  │       For each client:
  │         - If CGI active and timed out → send 504 Gateway Timeout
  │         - If idle > CLIENT_TIMEOUT → close connection
  │
  ├── 3. Session cleanup (every SESSION_CLEANUP_INTERVAL seconds)
  │       Remove expired sessions from SessionManager
  │
  ├── 4. For each FD with events:
  │   │
  │   ├── Is it a CGI pipe?
  │   │   ├── POLLOUT → handleCgiWrite(fd)  — feed request body to CGI stdin
  │   │   └── POLLIN/HUP/ERR → handleCgiRead(fd) — read CGI stdout
  │   │
  │   ├── Is it a server socket + POLLIN?
  │   │   └── acceptNewConnection(server)
  │   │       ├── Check MAX_CONNECTIONS limit
  │   │       ├── accept(server_fd) → client_fd
  │   │       ├── setNonBlocking(client_fd)
  │   │       ├── new Client(client_fd)
  │   │       └── pollManager.addFd(client_fd, POLLIN)
  │   │
  │   ├── Is it a client + POLLIN?
  │   │   └── handleClientRead(fd)
  │   │       ├── client->receiveData()  — read() into buffer
  │   │       ├── If 0 bytes → client disconnected, close
  │   │       └── processRequest(client, server)
  │   │
  │   ├── Is it a client + POLLOUT?
  │   │   └── handleClientWrite(fd)
  │   │       ├── client->sendData()  — write() from buffer
  │   │       ├── If send buffer empty:
  │   │       │   ├── keep-alive → switch to POLLIN (wait for next request)
  │   │       │   └── not keep-alive → close connection
  │   │       └── If write fails → close connection
  │   │
  │   └── HUP/ERR without IN/OUT?
  │       └── closeClientConnection(fd)
  │
  └── Loop back to poll()
```

### Critical Detail: FD Invalidation Safety
After handling a CGI pipe or closing a connection, the code checks if `pollManager.getFd(i) != fd`. If the FD at position `i` changed (because `removeFd` swaps the last element into the gap), the loop index `i` is decremented to re-process that position.

---

## 6. Request Lifecycle

### Phase 1: Receiving Data
```
Client::receiveData()
  │
  └── while (read(fd, buf, 4096) > 0)
        Append to storeReceiveData buffer
        Update lastActivity timestamp
```
The non-blocking `read()` loop drains all available data in one go. This is critical for handling large requests.

### Phase 2: Processing — `processRequest(client, server)`
This is called every time new data arrives. It handles the HTTP pipelining scenario (multiple requests in one buffer):

```
processRequest(client, server):
  │
  while (true):
    │
    ├── If headers NOT yet parsed:
    │   └── parseAndRouteHeaders(client, server)
    │       ├── Strip leading CRLF (between pipelined requests)
    │       ├── Look for "\r\n\r\n" (end of headers)
    │       ├── If not found → return (wait for more data)
    │       ├── Parse headers into HttpRequest
    │       ├── Determine keep-alive from Connection header
    │       ├── Remove header bytes from receive buffer
    │       ├── Create Router and call processRequest()
    │       ├── Set RouteResult in clientRoutes map
    │       ├── Check for errors (404, 405, etc.)
    │       ├── Validate body requirements
    │       └── If CGI: start CGI process immediately
    │
    ├── If headers parsed AND CGI active:
    │   └── handleCgiBodyStreaming(client)
    │       Feed incoming body data to CGI's stdin pipe
    │
    ├── If headers parsed AND regular request:
    │   └── handleRegularBody(client)
    │       ├── Wait for full body (Content-Length bytes)
    │       ├── Or decode chunked transfer encoding
    │       ├── Build response via ResponseBuilder
    │       └── finalizeResponse() → queue for sending
    │
    └── break (wait for more data or next poll cycle)
```

### Phase 3: Sending Response
```
handleClientWrite(fd):
  │
  ├── client->sendData()
  │   └── write(fd, buffer, size)
  │       Erase sent bytes from storeSendData
  │
  └── If buffer empty:
      ├── keep-alive → pollManager.addFd(fd, POLLIN)
      └── !keep-alive → closeClientConnection(fd)
```

---

## 7. Configuration Parsing

### Lexer → Token stream → Parser → ServerConfig objects

```
ConfigFile (text)
    │
    ▼
ConfigLexer (character-by-character)
    │ Produces tokens: WORD, STRING, SEMICOLON, LBRACE, RBRACE, EOF
    │ Strips comments (#...) and whitespace
    ▼
ConfigParser (recursive descent)
    │ parse()
    │   ├── parseHttp()     — optional http { } wrapper
    │   ├── parseServer()   — server { } blocks
    │   │   ├── Server directives: listen, server_name, root, index, etc.
    │   │   └── parseLocation() — nested location { } blocks
    │   │       └── Location directives: methods, root, cgi_pass, etc.
    │   └── validate()      — fill defaults, check required fields
    ▼
VectorServerConfig (ready for ServerManager)
```

### Config Inheritance Chain
```
http block (client_max_body_size)
  └── server block (inherits http max_body if not set)
      └── location block (inherits server root, indexes, max_body if not set)
```

The `validate()` method fills in defaults:
- Missing `client_max_body_size` → `DEFAULT_MAX_BODY_SIZE` (1MB)
- Missing location `root` → server's root
- Missing location `index` → server's index or `index.html`
- Missing location `methods` → `GET` only

### Directive Map Pattern
The parser uses function pointer maps to dispatch directive parsing:
```cpp
_serverDirectives["listen"]   = &ServerConfig::setListen;
_locationDirectives["methods"] = &LocationConfig::setAllowedMethods;
```
This is an elegant pattern that avoids long chains of if/else.

---

## 8. Routing

`Router::processRequest()` is the decision engine:

```
processRequest():
  │
  ├── 1. findServer()
  │   ├── Match by port AND server_name → exact match
  │   └── Fallback: getDefaultServer(port) → first server on that port
  │
  ├── 2. bestMatchLocation(locations)
  │   └── Longest prefix match on URI
  │       URI: /cgi-bin/test.py
  │       Locations: /, /cgi-bin, /uploads
  │       Winner: /cgi-bin (longest prefix match)
  │
  ├── 3. Check redirect (return 301 /target)
  │   └── If location has return directive → set redirect in RouteResult
  │
  ├── 4. Check method allowed
  │   └── Is request method in location's methods list?
  │
  ├── 5. CGI check
  │   └── If location has cgi_pass AND file extension matches:
  │       ├── resolveCgiScriptAndPathInfo()
  │       │   Walk URI segments to find actual script file
  │       │   Remaining segments become PATH_INFO
  │       └── Set HandlerType = CGI
  │
  ├── 6. Upload check
  │   └── If POST/PUT AND location has upload_dir:
  │       └── Set HandlerType = UPLOAD
  │
  ├── 7. Resolve filesystem path
  │   ├── root + (URI - location path) = filesystem path
  │   │   Example: root=./www, location=/subdir, URI=/subdir/file.html
  │   │   → ./www/file.html
  │   ├── If directory → try index files
  │   │   ├── Found index → serve it
  │   │   ├── No index + autoindex on → HandlerType = DIRECTORY_LISTING
  │   │   └── No index + autoindex off → 404
  │   └── If file → HandlerType = STATIC or DELETE_FILE
  │
  └── Return RouteResult (contains everything handlers need)
```

---

## 9. Response Building

`ResponseBuilder::build()` dispatches to the appropriate handler:

```
build(routeResult, cgi, openFds):
  │
  ├── Redirect? → Set Location header + redirect status
  ├── Error? → handleError() via ErrorPageHandler
  │
  └── Switch on HandlerType:
      │
      ├── DIRECTORY_LISTING → DirectoryListingHandler
      │   ├── opendir() / readdir() / closedir()
      │   ├── Build HTML table with file info
      │   └── Return styled HTML page
      │
      ├── UPLOAD → UploaderHandler
      │   ├── Parse multipart/form-data boundary
      │   ├── Extract filename and content
      │   ├── sanitizeFilename() (security: strip special chars)
      │   ├── Write to upload_dir
      │   └── Return 201 Created
      │
      ├── CGI → CgiHandler
      │   ├── (described in section 10)
      │   └── Response built asynchronously after CGI completes
      │
      ├── STATIC → StaticFileHandler
      │   ├── readFileContent(path)
      │   ├── MimeTypes::get(path) for Content-Type
      │   └── Set body (skip for HEAD requests)
      │
      └── DELETE_FILE → DeleteHandler
          ├── Security: reject paths with ".."
          ├── Verify it's a regular file
          ├── std::remove(path)
          └── Return JSON success message
```

### Error Page Resolution
```
ErrorPageHandler::handle():
  │
  ├── 1. Check location-level error_page
  ├── 2. Check server-level error_page
  └── 3. Fallback: generateHtml() → animated CSS/JS error page
```

---

## 10. CGI Handling

CGI is the most complex part. It runs asynchronously via pipes:

### Architecture
```
Parent (webserv)              Child (CGI script)
     │                              │
     │ pipe(parentToChild)          │
     │ pipe(childToParent)          │
     │                              │
     │    fork()                    │
     │──────────────────────────────│
     │                              │
     │ close read end               │ close write end
     │ of parentToChild             │ of parentToChild
     │                              │
     │ close write end              │ close read end
     │ of childToParent             │ of childToParent
     │                              │
     │ Set pipes non-blocking       │ dup2(p2c[0] → stdin)
     │ Register with poll()         │ dup2(c2p[1] → stdout)
     │                              │
     │ write body ──────────────────│→ stdin
     │ to parentToChild[1]          │
     │                              │ chdir(script_dir)
     │                              │ execve(interpreter, argv, envp)
     │                              │
     │ read output ←────────────────│ stdout
     │ from childToParent[0]        │
     │                              │
     │ parseOutput()                │ _exit()
     │ Build HttpResponse           │
     │ Send to client               │
```

### CGI Environment Variables
Built by `CgiHandler::buildEnv()`:
```
GATEWAY_INTERFACE = CGI/1.1
SERVER_NAME       = localhost
SERVER_PORT       = 8080
SERVER_PROTOCOL   = HTTP/1.1
REQUEST_METHOD    = GET/POST
REQUEST_URI       = /cgi-bin/test.py?key=val
QUERY_STRING      = key=val
SCRIPT_NAME       = /cgi-bin/test.py
SCRIPT_FILENAME   = /full/path/to/test.py
PATH_INFO         = /extra/path
PATH_TRANSLATED   = /www/extra/path
DOCUMENT_ROOT     = ./www/cgi-bin
CONTENT_TYPE      = application/x-www-form-urlencoded
CONTENT_LENGTH    = 42
REMOTE_ADDR       = 127.0.0.1
HTTP_*            = All request headers (transformed)
```

### CGI Lifecycle in Poll Loop
```
1. handleCgiWrite(pipeFd)
   ├── Write request body to CGI's stdin pipe
   ├── When done: close write pipe, remove from poll
   └── Non-blocking: write as much as possible per cycle

2. handleCgiRead(pipeFd)
   ├── Read CGI output from stdout pipe
   ├── When EOF (read returns 0):
   │   ├── Remove pipes from poll
   │   ├── Parse CGI output (headers + body)
   │   ├── Build HttpResponse
   │   ├── Queue response for client
   │   └── CgiProcess::finish() → waitpid
   └── Non-blocking: read as much as available

3. Timeout (checkTimeouts)
   └── If CGI runs > CGI_TIMEOUT (160s):
       ├── kill(pid, SIGKILL)
       ├── waitpid()
       └── Send 504 Gateway Timeout
```

### CGI Output Parsing
```
CgiHandler::parseOutput():
  │
  ├── Split on "\r\n\r\n" → headers + body
  ├── Parse headers line by line:
  │   ├── "Status: 200 OK" → set response status
  │   ├── "Set-Cookie: ..." → add cookie
  │   └── Other headers → add to response
  ├── If no Status header → default 200 OK
  └── Set body from remaining content
```

---

## 11. Session Management (Bonus)

### Session Flow
```
SessionManager
  │
  ├── createSession(userId) → generates 32-char GUID
  │   └── Uses /dev/urandom for cryptographic randomness
  │
  ├── getSession(sessionId)
  │   ├── Find in map
  │   ├── Check expiry (SESSION_TIMEOUT = 30 min)
  │   └── Update lastActivity
  │
  ├── cleanupExpiredSessions()
  │   └── Called every SESSION_CLEANUP_INTERVAL (60s) in main loop
  │
  └── Cookie handling:
      ├── buildSetCookieHeader(id) → "webserv_sid=<id>; Path=/; HttpOnly"
      └── buildExpiredCookieHeader() → expires cookie
```

**Note**: The SessionManager is initialized in ServerManager but not currently wired into the request flow for automatic session handling. It's available for CGI scripts to use via the cookie mechanism.

---

## 12. Clean Code Report

### Changes Applied

#### DRY (Don't Repeat Yourself)
- **`HEADER_SERVER` duplication**: Removed `response.addHeader(HEADER_SERVER, "Webserv/1.0")` from all 7 handlers. The `toString()` method now adds it automatically if not present.
- **`clientRoutes.erase()` duplication**: Moved into `sendErrorResponse()` so every error path automatically cleans up the route. Eliminated 9 redundant calls.

#### KISS (Keep It Simple, Stupid)
- **`HttpResponse::toString()` was non-const** and mutated headers. Now const-correct — it adds Date/Server headers during serialization without modifying the object.

#### YAGNI (You Ain't Gonna Need It) - Identified
- `SessionManager` is built but not integrated into request handling (it's bonus feature scaffolding).
- `findValueStrInMap` / `findValueIntInMap` duplicate the template `getValue<>` function.
- `isChunkedTransferEncoding()`, `extractContentLength()`, `getHeaderValue()` in Utils.cpp are not used by the main code (may be used by tests).
- `getListerToConfigs()` was declared but never implemented — removed.

#### Intention-Revealing Names
| Before | Issue |
|--------|-------|
| `storeReceiveData` | Could be `receiveBuffer` |
| `storeSendData` | Could be `sendBuffer` |
| `getDifferentTime` | Should be `getElapsedTime` |
| `g_running` | Could be `g_serverRunning` |

#### Avoid Magic Numbers — Fixed
| Magic Number | Replaced With |
|-------------|---------------|
| `10` in `pollConnections(10)` | `POLL_TIMEOUT_MS` (100) |
| `1024 * 1024` in ConfigParser | `DEFAULT_MAX_BODY_SIZE` |

#### Removed Dead Code
- Profanity comment in Client.cpp removed
- Commented-out HEAD method workaround in Router.cpp removed

#### Guard Clauses
The codebase already uses guard clauses well (early returns in most functions).

#### CQS (Command-Query Separation)
- `Logger::info()` / `Logger::error()` return `bool` — this is intentional to allow `return Logger::error("msg")` one-liners. Pragmatic trade-off.

### Remaining Opportunities
1. **SRP**: `ServerManager` (610 lines) handles too many responsibilities. Could extract: `ConnectionManager`, `CgiManager`, `TimeoutChecker`.
2. **SRP**: `Utils.cpp` (895 lines) is a utility dumping ground. Could split into: `StringUtils`, `FileUtils`, `PathUtils`, `HttpUtils`, `ParsingUtils`.
3. **ISP**: `IHandler` has a single method, which is good. But `CgiHandler` has a second `handle()` overload.

---

## 13. Performance Analysis

### Issues Found & Fixed

| Issue | Impact | Fix Applied |
|-------|--------|-------------|
| `pollConnections(10)` — 10ms timeout | CPU waste: 100 wakeups/sec when idle | Changed to `POLL_TIMEOUT_MS` (100ms) → 10 wakeups/sec |
| `findServerByFd()` — O(n) linear search | Slow with many servers | Added `serverFdMap` for O(1) lookup |
| `Logger::debug()` on every response | Console I/O blocks, counter increments | Removed from hot path (`toString()`) |
| Duplicate `HEADER_SERVER` additions | Unnecessary string operations per response | Centralized in `toString()` |

### Remaining Performance Considerations

| Issue | Impact | Recommendation |
|-------|--------|---------------|
| `String::erase(0, n)` in Client buffers | O(n) copy on every send/receive | Use offset pointer instead of erasing front |
| `checkTimeouts()` iterates ALL clients | O(n) every poll cycle | Use a priority queue sorted by timeout |
| `toLowerWords()` computed repeatedly | String allocation per call | Cache the result on first computation |
| `HttpResponse::toString()` string concatenation | Multiple allocations | Pre-calculate total size and `reserve()` |
| `MimeTypes` loaded from file on construction | Both `ServerManager::mimeTypes` AND `ResponseBuilder::mimeTypes` parse the file | Pass MimeTypes by reference, don't construct default |
| `RouteResult` copied frequently | Large object with `HttpRequest` member | Use pointers or references where possible |

---

## 14. File-by-File Walkthrough

### `src/main.cpp`
Entry point. Parses command-line arguments, creates `ConfigParser`, builds `ServerManager`, sets up signals, and starts the event loop. The try/catch ensures the server never crashes fatally.

### `src/config/ConfigLexer.cpp`
Character-by-character tokenizer for NGINX-style config files. Handles:
- Whitespace/newline skipping
- `#` comment stripping
- Quoted strings (`"..."` and `'...'`)
- Single-character tokens: `{`, `}`, `;`
- Words: sequences of non-delimiter characters

### `src/config/ConfigToken.cpp`
Simple token class with `type` (WORD, STRING, SEMICOLON, LBRACE, RBRACE, EOF), `value`, and `line` number for error reporting.

### `src/config/ConfigParser.cpp`
Recursive-descent parser. Uses function pointer maps (`ServerDirectiveMap`, `LocationDirectiveMap`) to dispatch directive parsing. The `validate()` method fills in defaults and checks required fields.

### `src/config/ServerConfig.cpp`
Data class for a server block. Stores listen addresses, locations, server names, root, indexes, max body size, and error pages. Each setter validates its input and rejects duplicates.

### `src/config/LocationConfig.cpp`
Data class for a location block. Stores path, root, autoindex, indexes, upload_dir, CGI mappings, allowed methods, redirect, and error pages. The `setCgiPass` method maps file extensions to interpreter paths.

### `src/config/ListenAddressConfig.cpp`
Simple pair of `(interface, port)`. E.g., `("0.0.0.0", 8080)`.

### `src/config/MimeTypes.cpp`
Parses `src/config/mime.types` file (NGINX format: `type ext1 ext2;`). Maps file extensions to MIME types. Falls back to `application/octet-stream`.

### `src/server/Server.cpp`
Encapsulates a single listening socket. `init()` creates, configures, binds, and listens on the socket. `acceptConnection()` accepts new clients and extracts the remote IP address.

### `src/server/PollManager.cpp`
Wrapper around `poll()`. Maintains a vector of `pollfd` structs and an index map for O(1) FD lookup. `addFd()` adds or updates a monitored FD. `removeFd()` swaps the last element into the gap (O(1) removal). `pollConnections()` calls `poll()` with the given timeout.

### `src/server/Client.cpp`
Represents a connected client. Holds receive/send buffers, the parsed `HttpRequest`, a `CgiProcess` (if CGI is active), keep-alive state, and the last activity timestamp. `receiveData()` does a non-blocking read loop. `sendData()` writes as much as possible.

### `src/server/ServerManager.cpp`
The central orchestrator (610 lines). Manages the poll loop, connection lifecycle, request processing, CGI pipe management, and timeout checking. See sections 5-6 for detailed flow.

### `src/http/HttpRequest.cpp`
HTTP request parser. Handles:
- Request line: `METHOD URI HTTP/1.1`
- Header parsing with case-insensitive keys
- URI decoding (percent-encoding)
- Query string and fragment extraction
- Content-Length and Transfer-Encoding validation
- Cookie parsing
- Host header validation (required for HTTP/1.1)
- Error codes set for each validation failure

### `src/http/HttpResponse.cpp`
HTTP response builder. Stores status code, headers, cookies, and body. `toString()` serializes to wire format, automatically adding `Date` and `Server` headers.

### `src/http/Router.cpp`
Request routing engine. Matches the request URI to the best location using longest-prefix matching. Determines the handler type (STATIC, CGI, UPLOAD, DELETE, DIRECTORY_LISTING). Resolves filesystem paths and CGI script/PATH_INFO separation.

### `src/http/RouteResult.cpp`
Data transfer object carrying the routing decision from Router to ResponseBuilder. Contains: status code, handler type, filesystem path, matched location, server config, and the original request.

### `src/http/ResponseBuilder.cpp`
Dispatches to the appropriate handler based on `HandlerType`. Handles multipart upload parsing, redirect responses, and error fallbacks. Also has `buildCgiResponse()` for parsing CGI output.

### `src/handlers/CgiHandler.cpp`
Creates the CGI child process. Sets up pipes, forks, configures the child's environment (stdin/stdout redirection, environment variables), changes to the script's directory, and calls `execve()`. The parent stores pipe FDs for non-blocking I/O.

### `src/handlers/CgiProcess.cpp`
Manages the CGI child process lifecycle. Handles non-blocking writes to the CGI's stdin pipe, non-blocking reads from stdout, process cleanup (SIGTERM → SIGKILL → waitpid), and output accumulation.

### `src/handlers/StaticFileHandler.cpp`
Reads a file from disk and serves it with the appropriate MIME type. Skips the body for HEAD requests.

### `src/handlers/UploaderHandler.cpp`
Writes uploaded file content to the upload directory. Sanitizes filenames (replacing special characters with underscores). Ensures the upload directory exists.

### `src/handlers/DeleteHandler.cpp`
Deletes a file from the filesystem. Validates the path doesn't contain `..` (directory traversal prevention) and that the target is a regular file.

### `src/handlers/DirectoryListingHandler.cpp`
Generates an HTML page listing directory contents. Uses `opendir`/`readdir`/`closedir` to enumerate entries. Creates a styled table with icons (from Flaticon CDN), file sizes, and last-modified dates.

### `src/handlers/FileHandler.cpp`
Helper class for directory listing entries. Determines whether an entry is a file or directory, calculates size, and generates the appropriate icon URL and link.

### `src/handlers/ErrorPageHandler.cpp`
Generates error responses. First checks location-level custom error pages, then server-level, then falls back to a generated animated HTML page with CSS animations and a counter effect.

### `src/utils/Utils.cpp`
895-line utility collection containing:
- **Time functions**: `getCurrentTime`, `formatDateTime` (manual date formatting without `gmtime`)
- **String functions**: `toUpperWords`, `toLowerWords`, `trimSpaces`, `htmlEntities`, `splitByChar`, `splitByString`, `urlDecode`
- **File functions**: `fileExists`, `getFileType`, `readFileContent`, `sanitizeFilename`
- **Path functions**: `normalizePath` (resolves `.` and `..`), `joinPaths`, `pathStartsWith`, `getUriRemainder`
- **HTTP helpers**: `checkAllowedMethods`, `setNonBlocking`, `getHttpStatusMessage`, `convertMaxBodySize`
- **Body parsing**: `extractBoundaryFromContentType`, `parseMultipartFormData`, `decodeChunkedBody`
- **GUID generation**: Uses `/dev/urandom` for session IDs

### `src/utils/Constants.hpp`
All magic numbers as named `#define` constants. HTTP status codes, header names, method names, timeouts, buffer sizes, etc.

### `src/utils/Types.hpp`
Typedefs for commonly used types: `String`, `VectorString`, `MapString`, `VectorServerConfig`, etc. Reduces verbosity throughout the codebase.

### `src/utils/Enums.hpp`
Enumerations: `Type` (token types), `FileType` (file/directory/unknown), `HandlerType` (STATIC/CGI/UPLOAD/etc.).

### `src/utils/Logger.cpp`
Simple colored output logger. `info()` prints blue to stdout, `error()` prints red to stderr, `debug()` prints cyan with a counter. All return `bool` for one-liner error returns.

### `src/utils/SessionManager.cpp`
Session store using an in-memory map. Creates sessions with cryptographic GUIDs, supports timeout-based expiry, session regeneration (security), and Set-Cookie header generation.

### `src/utils/SessionResult.cpp`
Individual session data. Stores session ID, user ID, creation time, last activity, and a key-value data map.

---

## Summary of Request-to-Response Data Flow

```
Browser sends: GET /cgi-bin/test.py?name=world HTTP/1.1\r\n
               Host: localhost:8080\r\n\r\n

1. poll() signals POLLIN on client FD
2. read() → storeReceiveData buffer
3. Find "\r\n\r\n" → header complete
4. HttpRequest::parseHeaders()
   - method = "GET", uri = "/cgi-bin/test.py", query = "name=world"
5. Router::processRequest()
   - findServer() → ServerConfig on port 8080
   - bestMatchLocation() → location /cgi-bin (longest prefix)
   - isCgiRequest("/www/cgi-bin/test.py", loc) → .py matches → CGI!
   - HandlerType = CGI
6. No body needed (GET) → CGI write done
7. CgiHandler::handle()
   - pipe() × 2, fork()
   - Child: chdir, dup2, execve("/usr/bin/python3", "test.py")
   - Parent: register pipes with poll()
8. poll() signals POLLIN on CGI read pipe
9. handleCgiRead() → accumulate output
10. EOF on pipe → parse CGI output
11. buildCgiResponse() → HttpResponse
12. Queue response in client's sendBuffer
13. poll() signals POLLOUT on client FD
14. handleClientWrite() → write() response to socket
15. Keep-alive → switch to POLLIN, wait for next request
```
