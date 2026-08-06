#include "ws/server.hpp"

#include <boost/asio.hpp>
#include <fmt/core.h>

#include <algorithm>
#include <cstdlib>
#include <thread>
#include <vector>

namespace   asio    = boost::asio;
using       tcp     = asio::ip::tcp;

constexpr int MAX_CONCURRENT_SESSIONS = 10;

int main(int argc, char* argv[])
{
    try {
        const auto address =
            asio::ip::make_address("0.0.0.0");

        unsigned short port = 8080;

        if (argc > 1) {
            port = static_cast<unsigned short>(
                std::atoi(argv[1]));
        }

        asio::io_context ioc;

        tcp::endpoint endpoint{address, port};

        ws::server server{
            ioc,
            endpoint,
            MAX_CONCURRENT_SESSIONS
        };

        server.run();

        const unsigned int thread_count =
            std::max(
                2u,
                std::thread::hardware_concurrency());

        fmt::print(
            "Starting {} worker threads\n",
            thread_count);

        std::vector<std::thread> threads;
        threads.reserve(thread_count - 1);

        for (unsigned int i = 0;
             i < thread_count - 1;
             ++i)
        {
            threads.emplace_back(
                [&ioc]
                {
                    ioc.run();
                });
        }

        ioc.run();

        for (auto& thread : threads) {
            thread.join();
        }
    }
    catch (const std::exception& ex) {
        fmt::print("Fatal error: {}\n", ex.what());

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}