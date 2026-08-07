#include "logging/chat_logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace logging
{
    chat_logger::chat_logger()
    {
        std::filesystem::create_directories("logs");
    }

    std::string chat_logger::current_log_file() const
    {
        const auto now =
            std::chrono::system_clock::now();

        const std::time_t time =
            std::chrono::system_clock::to_time_t(now);

        std::tm local_time{};

    #ifdef _WIN32
        localtime_s(&local_time, &time);
    #else
        localtime_r(&time, &local_time);
    #endif

        std::ostringstream filename;

        filename
            << "logs/chat_"
            << std::put_time(
                &local_time,
                "%Y-%m-%d_%H-%M") // "%Y-%m-%d_%H"
            << ".log";

        return filename.str();
    }

    void chat_logger::log(
        const std::string& username,
        const std::string& message)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto now =
            std::chrono::system_clock::now();

        const std::time_t time =
            std::chrono::system_clock::to_time_t(now);

        std::tm local_time{};

    #ifdef _WIN32
        localtime_s(&local_time, &time);
    #else
        localtime_r(&time, &local_time);
    #endif

        std::ofstream file(
            current_log_file(),
            std::ios::app);

        if (!file.is_open()) {
            return;
        }

        file
            << "["
            << std::put_time(
                &local_time,
                "%H:%M:%S")
            << "] "
            << username
            << ": "
            << message
            << '\n';
    }
} // namespace logging