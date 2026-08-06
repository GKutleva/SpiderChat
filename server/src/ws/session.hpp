#pragma once

#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <atomic>
#include <memory>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

namespace ws {

class session : public std::enable_shared_from_this<session> {
public:
    explicit session(tcp::socket socket, std::atomic<int>& active_sessions);
    ~session();
    void run();

private:
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::atomic<int>& active_sessions_;

    void do_read();
    void close_and_count_down();
};

} // namespace ws
