#include "ws/server.hpp"
#include "ws/session.hpp"

#include <boost/asio/strand.hpp>
#include <fmt/core.h>
#include "chat/chat_room.hpp"

namespace ws {

server::server(
    asio::io_context& ioc,
    tcp::endpoint endpoint,
    int max_sessions)
    : ioc_(ioc),
      acceptor_(ioc, endpoint),
      chat_room_(std::make_shared<chat::chat_room>()),
      max_sessions_(max_sessions)
{
}

void server::run()
{
    const auto endpoint = acceptor_.local_endpoint();

    fmt::print(
        "Server started on {}:{}\n",
        endpoint.address().to_string(),
        endpoint.port());

    do_accept();
}

void server::do_accept()
{
    acceptor_.async_accept(
        asio::make_strand(ioc_),
        [this](beast::error_code ec, tcp::socket socket)
        {
            if (ec) {
                fmt::print(
                    "Accept error: {}\n",
                    ec.message());
            }
            else if (active_sessions_.load() >= max_sessions_) {
                reject_connection(socket);
            }
            else {
                start_session(std::move(socket));
            }

            do_accept();
        });
}

void server::start_session(tcp::socket socket)
{
    active_sessions_.fetch_add(1);

    fmt::print(
        "New client connected. Active sessions: {}\n",
        active_sessions_.load());

    auto new_session = std::make_shared<session>(
        std::move(socket),
        active_sessions_,
        chat_room_);

    new_session->run();
}

void server::reject_connection(tcp::socket& socket)
{
    fmt::print(
        "Connection rejected. Server limit is {} clients.\n",
        max_sessions_);

    beast::error_code ec;
    socket.close(ec);

    if (ec) {
        fmt::print(
            "Error closing connection: {}\n",
            ec.message());
    }
}

} // namespace ws