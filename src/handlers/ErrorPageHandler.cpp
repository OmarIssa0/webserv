#include "ErrorPageHandler.hpp"
#include <fstream>
#include <sstream>

std::string ErrorPageHandler::generateHtml(int code, const std::string& msg) const {
    return "<!DOCTYPE html>\n"
           "<html lang=\"en\">\n"
           "<head>\n"
           "<meta charset=\"UTF-8\">\n"
           "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
           "<title>" +
           typeToString<int>(code) + " " + msg +
           "</title>\n"
           "<style>\n"
           "body{font-family:system-ui;background:#0f172a;color:#e2e8f0;display:flex;"
           "justify-content:center;align-items:center;min-height:100vh;margin:0;}\n"
           ".box{text-align:center;}\n"
           "h1{font-size:6rem;margin:0;color:#f43f5e;}\n"
           "p{font-size:1.5rem;color:#94a3b8;}\n"
           "</style>\n"
           "</head>\n"
           "<body>\n"
           "<div class=\"box\">\n"
           "<h1>" +
           typeToString<int>(code) +
           "</h1>\n"
           "<p>" +
           msg +
           "</p>\n"
           "</div>\n"
           "</body>\n"
           "</html>\n";
}

void ErrorPageHandler::handle(HttpResponse& response, const RouteResult& resultRouter, const MimeTypes& mimeTypes) const {
    int         code = resultRouter.getStatusCode() ? resultRouter.getStatusCode() : 500;
    std::string msg  = resultRouter.getErrorMessage().empty() ? getHttpStatusMessage(code) : resultRouter.getErrorMessage();

    std::string body;
    // Check for custom error page in location first, then server
    const std::string customPage = resultRouter.getLocation() ? resultRouter.getLocation()->getErrorPage(code) : "";
    if (!customPage.empty() && readFileContent(customPage, body)) {
        response.setStatus(code, msg);
        response.addHeader("Content-Type", mimeTypes.get(customPage));
        response.addHeader("Content-Length", typeToString<size_t>(body.size()));
        response.setBody(body);
        return;
    }

    const std::string serverPage = resultRouter.getServer() ? resultRouter.getServer()->getErrorPage(code) : "";
    if (!serverPage.empty() && readFileContent(serverPage, body)) {
        response.setStatus(code, msg);
        response.addHeader("Content-Type", mimeTypes.get(serverPage));
        response.addHeader("Content-Length", typeToString<size_t>(body.size()));
        response.setBody(body);
        return;
    }

    // Fallback HTML
    body = generateHtml(code, msg);
    response.setStatus(code, msg);
    response.addHeader("Content-Type", "text/html");
    response.addHeader("Content-Length", typeToString<size_t>(body.size()));
    response.setBody(body);
}
