#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/signal_set.hpp>
#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include "ws/server.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

// Maximum concurrent websocket sessions
static constexpr int MAX_CONCURRENT_SESSIONS = 10;


int main(int argc, char* argv[])
{
    try {
        auto const address = asio::ip::make_address("0.0.0.0");
        unsigned short port = 8080;
        if (argc > 1) {
            port = static_cast<unsigned short>(std::atoi(argv[1]));
        }

        asio::io_context ioc{1};

        // Use the central ws::server implementation instead of duplicating accept/session handling.
        tcp::endpoint endpoint{address, port};
        ws::server srv{ioc, endpoint, MAX_CONCURRENT_SESSIONS};
        srv.run();

        unsigned int threads = std::max(2u, std::thread::hardware_concurrency());
        fmt::print("Starting {} worker threads\n", threads);
        std::vector<std::thread> v;
        v.reserve(threads - 1);
        for (unsigned int i = 0; i < threads - 1; ++i)
            v.emplace_back([&ioc] { ioc.run(); });
        ioc.run();
        for (auto& t : v)
            t.join();

    } catch (const std::exception& ex) {
        fmt::print("Fatal error: {}\n", ex.what());
        return EXIT_FAILURE;
    }

    
    return EXIT_SUCCESS;
}
