#pragma once

#include <mutex>
#include <string>

namespace logging
{

class chat_logger
{
public:
    chat_logger();

    void log(
        const std::string& username,
        const std::string& message);

private:
    std::string current_log_file() const;

private:
    std::mutex mutex_;
};

} // namespace logging