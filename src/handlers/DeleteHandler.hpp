#ifndef DELETE_HANDLER_HPP
#define DELETE_HANDLER_HPP
#include "../http/HttpResponse.hpp"
#include "../http/RouteResult.hpp"
#include "../utils/Utils.hpp"
#include "IHandler.hpp"

class DeleteHandler : public IHandler {
   public:
    DeleteHandler();
    DeleteHandler(const DeleteHandler& other);
    DeleteHandler& operator=(const DeleteHandler& other);
    ~DeleteHandler();

    bool handle(const RouteResult& resultRouter, HttpResponse& response) const;
};

#endif
