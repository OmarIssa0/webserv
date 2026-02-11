#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP
#include "../utils/Utils.hpp"
class SessionManager {
   public:
    SessionManager();
    SessionManager(const SessionManager& other);
    SessionManager& operator=(const SessionManager& other);
    ~SessionManager();

   private:
    String    id;
    time_t    createdTime;
    time_t    lastAccessTime;
    MapString data;
};
#endif