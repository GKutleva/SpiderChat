#include "chat/chat_room.hpp"
#include "ws/session.hpp"

#include <algorithm>
#include <fmt/core.h>

namespace chat 
{
    /*void chat_room::join(const std::shared_ptr<ws::session>& client)
    {  
        std::lock_guard<std::mutex> lock(mutex_);

        clients_.push_back(client);

        fmt::print(
            "Client joined chat room. Clients: {}\n",
             clients_.size());
    }*/

    // Additional methods for chat_room can be added here if needed.
    void chat_room::join(const std::shared_ptr<ws::session>& client)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            clients_.push_back(client);
        }

        list_clients();
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

    bool chat_room::register_username(const std::string& username)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        fmt::print(
            "Trying to register '{}'. Registered usernames: {}\n",
            username,
            usernames_.size());

        if (usernames_.find(username) != usernames_.end()) {
            fmt::print(
                "Username '{}' is already taken\n",
                username);

            return false;
        }

        usernames_.insert(username);

        fmt::print(
            "Username '{}' registered. Total usernames: {}\n",
            username,
            usernames_.size());

        return true;
    }

    void chat_room::unregister_username(const std::string& username)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        fmt::print(
            "Unregistering username '{}'\n",
            username);

        usernames_.erase(username);

        fmt::print(
            "Registered usernames remaining: {}\n",
            usernames_.size());
    }

    void chat_room::list_clients()
    {
        std::vector<std::shared_ptr<ws::session>> connected_clients;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto it = clients_.begin(); it != clients_.end();)
            {
                if (auto client = it->lock()) {
                    connected_clients.push_back(client);
                    ++it;
                }
                else {
                    it = clients_.erase(it);
                }
            }
        }

        fmt::print("\nConnected clients:\n");
        fmt::print("---------------------------------------------\n");
        fmt::print("{:<15} {:<18} {:<10}\n",
            "Username",
            "IP",
            "Online");
        fmt::print("---------------------------------------------\n");

        for (const auto& client : connected_clients)
        {
            const auto total_seconds =
                client->online_time().count();

            const auto hours = total_seconds / 3600;
            const auto minutes = (total_seconds % 3600) / 60;
            const auto seconds = total_seconds % 60;

            fmt::print(
                "{:<15} {:<18} {:02}:{:02}:{:02}\n",
                client->username(),
                client->ip_address(),
                hours,
                minutes,
                seconds);
        }

        fmt::print("---------------------------------------------\n");
        fmt::print("Total clients: {}\n\n", connected_clients.size());
    }

    bool chat_room::kick(const std::string& username)
    {
        std::shared_ptr<ws::session> client_to_kick;
        {
            std::lock_guard<std::mutex> lock(mutex_);

            for (auto it = clients_.begin(); it != clients_.end();)
            {
                if (auto client = it->lock()) {
                    if (client->username() == username) {
                        client_to_kick = client;
                        break;
                    }

                    ++it;
                }
                else {
                    it = clients_.erase(it);
                }
            }
        }

        if (!client_to_kick) {
            return false;
        }

        client_to_kick->kick();

        return true;
    }
} 