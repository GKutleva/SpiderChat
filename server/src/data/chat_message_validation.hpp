#pragma once

#include <string>
#include "chat_message.pb.h"

// Validate that required fields in ChatMessage are present and non-empty.
// Returns empty string on success, or an error message describing the validation failure.
inline std::string validateChatMessage(const chat::ChatMessage& m) {
    if (m.username().empty()) return "userName is required";
    if (m.message().empty()) return "message is required";
    if (m.ip().empty()) return "IP is required";
    return std::string();
}
