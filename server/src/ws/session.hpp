#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

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

    class session : public std::enable_shared_from_this<session>
    {
    public:    
    session(
        tcp::socket socket,
        std::atomic<int>& active_sessions,
        std::shared_ptr<chat::chat_room> chat_room);

    void run();
    void send_message(std::string message);

    private:
        void read_message();
        void process_message(const std::string& data);
        void write_next_message();
        void send_error(const std::string& error_message);
        void close();

    private:
        websocket::stream<tcp::socket> websocket_;
        beast::flat_buffer read_buffer_;

        std::atomic<int>& active_sessions_;
        std::atomic_bool closed_{false};

        std::shared_ptr<chat::chat_room> chat_room_;
        std::deque<std::string> write_queue_;
        asio::strand<asio::any_io_executor> strand_;
    };

} // namespace ws