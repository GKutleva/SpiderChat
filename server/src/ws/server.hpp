#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <atomic>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace ws {

class server {
public:
    server(asio::io_context& ioc, tcp::endpoint endpoint, int max_sessions = 10);
    void run();

private:
    asio::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::atomic<int> active_sessions_;
    int max_sessions_;

    void do_accept();
};

} // namespace ws
