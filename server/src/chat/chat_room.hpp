#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_set>

namespace ws 
{
    class session;
}

namespace chat 
{
    class chat_room
    {
    public:
        void join(const std::shared_ptr<ws::session>& client);
        void leave(const ws::session* client);

        void broadcast(const std::string& message);

        bool register_username(const std::string& username);
        void unregister_username(const std::string& username);

        void list_clients();
        bool kick(const std::string& username);

    private:
        std::vector<std::weak_ptr<ws::session>> clients_;
        std::unordered_set<std::string> usernames_;

        std::mutex mutex_;
    };
}