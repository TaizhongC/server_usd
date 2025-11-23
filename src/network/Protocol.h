#pragma once

#include <optional>
#include <string>
#include <vector>
#include <cstddef>

namespace protocol {

enum class CommandType {
  RequestLayers,
  Unknown,
};

struct Command {
  CommandType type = CommandType::Unknown;
  std::string action;
};

struct MeshPayload {
  std::vector<float> points;
  std::vector<int> counts;
  std::vector<int> indices;
  std::vector<float> colors;
};

// Parse an inbound text WebSocket payload into a command.
std::optional<Command> parse_command(const std::string& text);

// Outbound encoders.
std::string encode_layers(const std::vector<std::string>& layers);
std::string encode_stage_info(double meters_per_unit, const std::string& up_axis);
std::string encode_mesh_header(std::size_t vertex_count,
                               std::size_t face_count);
std::string encode_mesh_payload(const MeshPayload& mesh);
std::string encode_ack(const std::string& action);
std::string encode_error(const std::string& reason);

}  // namespace protocol
