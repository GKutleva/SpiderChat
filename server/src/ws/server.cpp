#include "ws/server.hpp"
#include "ws/session.hpp"
#include <fmt/core.h>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

namespace ws {

server::server(asio::io_context& ioc, tcp::endpoint endpoint, int max_sessions)
    : ioc_(ioc), acceptor_(ioc, endpoint), active_sessions_(0), max_sessions_(max_sessions)
{
    acceptor_.set_option(asio::socket_base::reuse_address(true));
}

void server::run()
{
    fmt::print("ws::server starting on {}:{}\n", acceptor_.local_endpoint().address().to_string(), acceptor_.local_endpoint().port());
    do_accept();
}

void server::do_accept()
{
    acceptor_.async_accept([this](boost::beast::error_code ec, tcp::socket socket) {
        if (ec) {
            fmt::print("Accept failed: {}\n", ec.message());
        } else {
            int current = active_sessions_.load();
            if (current >= max_sessions_) {
                // Send a short-lived busy response (text)
                try {
                    auto busy_ws = std::make_shared<boost::beast::websocket::stream<tcp::socket>>(std::move(socket));
                    busy_ws->async_accept([busy_ws](boost::beast::error_code ec) {
                        if (!ec) {
                            nlohmann::json reply;
                            reply["status"] = "error";
                            reply["message"] = "server overloaded";
                            std::string out = reply.dump();
                            busy_ws->text(true);
                            busy_ws->async_write(asio::buffer(out), [busy_ws](boost::beast::error_code, std::size_t) {
                                boost::beast::error_code e2;
                                busy_ws->next_layer().shutdown(tcp::socket::shutdown_both, e2);
                                busy_ws->next_layer().close(e2);
                            });
                        }
                    });
                } catch (const std::exception& ex) {
                    fmt::print("Error sending busy response: {}\n", ex.what());
                }
            } else {
                active_sessions_.fetch_add(1);
                auto s = std::make_shared<session>(std::move(socket), active_sessions_);
                s->run();
            }
        }

        // continue accepting
        do_accept();
    });
}

} // namespace ws
