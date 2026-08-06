#include "ws/session.hpp"
#include "chat_message.pb.h"
#include "data/chat_message_validation.hpp"
#include <google/protobuf/util/json_util.h>
#include <fmt/core.h>

namespace ws {

session::session(tcp::socket socket, std::atomic<int>& active_sessions)
    : ws_(std::move(socket)), active_sessions_(active_sessions)
{
}

session::~session() = default;

void session::run()
{
    auto self = shared_from_this();
    ws_.async_accept([self](beast::error_code ec) {
        if (ec) {
            fmt::print("Accept error: {}\n", ec.message());
            self->close_and_count_down();
            return;
        }
        self->do_read();
    });
}

void session::do_read()
{
    auto self = shared_from_this();
    ws_.async_read(buffer_, [self](beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec == websocket::error::closed) {
            self->close_and_count_down();
            return;
        }
        if (ec) {
            fmt::print("Read error: {}\n", ec.message());
            self->close_and_count_down();
            return;
        }

        std::string msg = beast::buffers_to_string(self->buffer_.data());
        self->buffer_.consume(self->buffer_.size());

        try {
            bool was_text = self->ws_.got_text();
            chat::ChatMessage proto;

            if (was_text) {
                auto status = google::protobuf::util::JsonStringToMessage(msg, &proto);
                if (!status.ok()) {
                    fmt::print("Protobuf JSON parse error: {}\n", status.ToString());
                    json reply;
                    reply["status"] = "error";
                    reply["message"] = "invalid json/protobuf";
                    std::string out = reply.dump();
                    self->ws_.text(true);
                    self->ws_.async_write(asio::buffer(out), [self](beast::error_code ec, std::size_t) {
                        if (ec) {
                            fmt::print("Write error: {}\n", ec.message());
                            self->close_and_count_down();
                            return;
                        }
                        self->do_read();
                    });
                    return;
                }
            } else {
                if (!proto.ParseFromString(msg)) {
                    fmt::print("Failed to parse protobuf binary frame\n");
                    json reply;
                    reply["status"] = "error";
                    reply["message"] = "invalid protobuf";
                    std::string out = reply.dump();
                    self->ws_.text(true);
                    self->ws_.async_write(asio::buffer(out), [self](beast::error_code ec, std::size_t) {
                        if (ec) {
                            fmt::print("Write error: {}\n", ec.message());
                            self->close_and_count_down();
                            return;
                        }
                        self->do_read();
                    });
                    return;
                }
            }

            // Validate required fields
            std::string v_err = validateChatMessage(proto);
            if (!v_err.empty()) {
                fmt::print("Validation failed: {}\n", v_err);
                json reply;
                reply["status"] = "error";
                reply["message"] = v_err;
                std::string out = reply.dump();
                self->ws_.text(true);
                self->ws_.async_write(asio::buffer(out), [self](beast::error_code ec, std::size_t) {
                    if (ec) {
                        fmt::print("Write error: {}\n", ec.message());
                        self->close_and_count_down();
                        return;
                    }
                    self->do_read();
                });
                return;
            }

            fmt::print("Session received: {} / {} / {}\n", proto.username(), proto.ip(), proto.message());

            // Reply in same format as received
            if (was_text) {
                std::string out_json;
                auto s2 = google::protobuf::util::MessageToJsonString(proto, &out_json);
                if (!s2.ok()) {
                    fmt::print("Failed to convert proto to JSON: {}\n", s2.ToString());
                    self->do_read();
                    return;
                }
                self->ws_.text(true);
                self->ws_.async_write(asio::buffer(out_json), [self](beast::error_code ec, std::size_t) {
                    if (ec) {
                        fmt::print("Write error (json reply): {}\n", ec.message());
                        self->close_and_count_down();
                        return;
                    }
                    self->do_read();
                });
            } else {
                std::string proto_bytes;
                if (!proto.SerializeToString(&proto_bytes)) {
                    fmt::print("Failed to serialize proto message\n");
                    self->do_read();
                    return;
                }
                self->ws_.binary(true);
                self->ws_.async_write(asio::buffer(proto_bytes), [self](beast::error_code ec, std::size_t) {
                    if (ec) {
                        fmt::print("Write error (proto reply): {}\n", ec.message());
                        self->close_and_count_down();
                        return;
                    }
                    self->do_read();
                });
            }

        } catch (const std::exception& ex) {
            fmt::print("Session exception: {}\n", ex.what());
            self->close_and_count_down();
            return;
        }
    });
}

void session::close_and_count_down()
{
    int prev = active_sessions_.fetch_sub(1);
    if (prev <= 0) {
        active_sessions_.store(0);
    }
    beast::error_code ec;
    ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
    ws_.next_layer().close(ec);
}

} // namespace ws
