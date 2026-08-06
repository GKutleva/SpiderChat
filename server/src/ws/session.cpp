#include "ws/session.hpp"

#include "chat_message.pb.h"
#include "data/chat_message_validation.hpp"

#include <fmt/core.h>
#include <google/protobuf/util/json_util.h>
#include <nlohmann/json.hpp>
#include "chat/chat_room.hpp"
#include <boost/asio/post.hpp>


#include <utility>

namespace ws 
{

using json = nlohmann::json;

session::session(
    tcp::socket socket,
    std::atomic<int>& active_sessions,
    std::shared_ptr<chat::chat_room> chat_room)
    : websocket_(std::move(socket)),
      strand_(websocket_.get_executor()),
      active_sessions_(active_sessions),
      chat_room_(std::move(chat_room))
{
}

void session::run()
{
    auto self = shared_from_this();

    websocket_.async_accept(
        [self](beast::error_code error)
        {
            if (error) {
                fmt::print(
                    "WebSocket handshake failed: {}\n",
                    error.message());

                self->close();
                return;
            }

            fmt::print("WebSocket connection established\n");

            self->chat_room_->join(self);
            self->read_message();
        });
}

void session::read_message()
{
    auto self = shared_from_this();

    websocket_.async_read(
        read_buffer_,
        [self](
            beast::error_code error,
            std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (error == websocket::error::closed) {
                self->close();
                return;
            }

            if (error) {
                fmt::print(
                    "WebSocket read failed: {}\n",
                    error.message());

                self->close();
                return;
            }

            if (!self->websocket_.got_text()) {
                self->read_buffer_.consume(
                    self->read_buffer_.size());

                self->send_error(
                    "Only text messages are supported");

                return;
            }

            std::string data =
                beast::buffers_to_string(
                    self->read_buffer_.data());

            self->read_buffer_.consume(
                self->read_buffer_.size());

            self->process_message(data);
        });
}

void session::process_message(const std::string& data)
{
    chat::ChatMessage message;

    const auto parse_result =
        google::protobuf::util::JsonStringToMessage(
            data,
            &message);

    if (!parse_result.ok()) {
        fmt::print(
            "Invalid message: {}\n",
            parse_result.ToString());

        send_error("Invalid message format");
        return;
    }

    const std::string validation_error =
        validateChatMessage(message);

    if (!validation_error.empty()) {
        fmt::print(
            "Message validation failed: {}\n",
            validation_error);

        send_error(validation_error);
        return;
    }

    fmt::print(
        "{}: {}\n",
        message.username(),
        message.message());

    std::string response;

    const auto serialization_result =
        google::protobuf::util::MessageToJsonString(
            message,
            &response);

    if (!serialization_result.ok()) {
        fmt::print(
            "Message serialization failed: {}\n",
            serialization_result.ToString());

        send_error("Could not create response");
        return;
    }

    chat_room_->broadcast(response);
    read_message();
}

void session::send_message(std::string message)
{
    auto self = shared_from_this();

    asio::post(
        strand_,
        [self, message = std::move(message)]() mutable
        {
            const bool write_in_progress =
                !self->write_queue_.empty();

            self->write_queue_.push_back(
                std::move(message));

            if (!write_in_progress) {
                self->write_next_message();
            }
        });
}

void session::write_next_message()
{
    auto self = shared_from_this();

    websocket_.text(true);

    websocket_.async_write(
        asio::buffer(write_queue_.front()),
        [self](
            beast::error_code error,
            std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (error) {
                fmt::print(
                    "WebSocket write failed: {}\n",
                    error.message());

                self->close();
                return;
            }

            self->write_queue_.pop_front();

            if (!self->write_queue_.empty()) {
                self->write_next_message();
            }
        });
}

void session::send_error(
    const std::string& error_message)
{
    json response;

    response["status"] = "error";
    response["message"] = error_message;

    send_message(response.dump());
}

void session::close()
{
    if (closed_.exchange(true)) {
        return;
    }

    chat_room_->leave(this);
    active_sessions_.fetch_sub(1);

    beast::error_code error;

    websocket_.next_layer().shutdown(
        tcp::socket::shutdown_both,
        error);

    error.clear();
    websocket_.next_layer().close(error);

    fmt::print(
        "Session closed. Active sessions: {}\n",
        active_sessions_.load());
}

} // namespace ws