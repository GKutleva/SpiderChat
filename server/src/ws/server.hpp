#pragma once

#include <atomic>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <memory>
#include <deque>

namespace chat 
{
    class chat_room;
}

namespace ws 
{
    namespace asio      = boost::asio;
    namespace beast     = boost::beast;
    namespace websocket = beast::websocket;

    using tcp = asio::ip::tcp;

    namespace asio  = boost::asio;
    namespace beast = boost::beast;

    using tcp = asio::ip::tcp;

    class server
    {
    public:
        server(
            asio::io_context& ioc,
            tcp::endpoint endpoint,
            int max_sessions);

        void run();

    private:
        void do_accept();
        void start_session(tcp::socket socket);
        void reject_connection(tcp::socket& socket);

    private:
        asio::io_context& ioc_;
        tcp::acceptor acceptor_;

        std::shared_ptr<chat::chat_room> chat_room_;

        std::atomic<int> active_sessions_{0};
        int max_sessions_;
    };

} // namespace ws