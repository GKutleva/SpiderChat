#include "chat/chat_room.hpp"
#include "ws/session.hpp"

#include <algorithm>
#include <fmt/core.h>

namespace chat 
{
    void chat_room::join(const std::shared_ptr<ws::session>& client)
    {  
        std::lock_guard<std::mutex> lock(mutex_);

        clients_.push_back(client);

        fmt::print(
            "Client joined chat room. Clients: {}\n",
             clients_.size());
    }

    void chat_room::leave(const ws::session* client)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        clients_.erase(
            std::remove_if(
                clients_.begin(),
                clients_.end(),
                [client](const std::weak_ptr<ws::session>& weak_client)
                {
                    const auto current = weak_client.lock();

                    return !current || current.get() == client;
                }),
            clients_.end());

        fmt::print(
            "Client left chat room. Clients: {}\n",
            clients_.size());
    }

    void chat_room::broadcast(const std::string& message)
    {
        std::vector<std::shared_ptr<ws::session>> connected_clients;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto iterator = clients_.begin();
                iterator != clients_.end();)
            {
                if (auto client = iterator->lock()) {
                    connected_clients.push_back(client);
                    ++iterator;
                }
                else {
                    iterator = clients_.erase(iterator);
                }
            }
        }

        for (const auto& client : connected_clients) {
            client->send_message(message);
        }
    }
} 