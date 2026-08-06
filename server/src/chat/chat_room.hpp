#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <vector>

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

    private:
        std::vector<std::weak_ptr<ws::session>> clients_;
        std::mutex mutex_;
    };
}