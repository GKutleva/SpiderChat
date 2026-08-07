#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>

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
    void kick(); 
    
    const std::string& username() const;

    const std::string& ip_address() const;
    std::chrono::seconds online_time() const;
     
    private:
        void read_message();
        void process_message(const std::string& data);
        void write_next_message();
        void send_error(const std::string& error_message);
        void close();

        void reset_inactivity_timer();

    private:
        websocket::stream<tcp::socket> websocket_;
        beast::flat_buffer read_buffer_;

        std::atomic<int>& active_sessions_;
        std::atomic_bool closed_{false};

        std::shared_ptr<chat::chat_room> chat_room_;
        std::deque<std::string> write_queue_;

        asio::strand<asio::any_io_executor> strand_;

        std::string username_;
        bool identified_ = false;

        std::string ip_address_;
        std::chrono::steady_clock::time_point connected_at_;

        asio::steady_timer inactivity_timer_;
    };

} // namespace ws