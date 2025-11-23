#pragma once

#include <optional>
#include <string>
#include <vector>

namespace protocol {

enum class CommandType {
  RequestLayers,
  Unknown,
};

struct Command {
  CommandType type = CommandType::Unknown;
  std::string action;
};

// Parse an inbound text WebSocket payload into a command.
std::optional<Command> parse_command(const std::string& text);

// Outbound encoders.
std::string encode_layers(const std::vector<std::string>& layers);
std::string encode_ack(const std::string& action);
std::string encode_error(const std::string& reason);

}  // namespace protocol
