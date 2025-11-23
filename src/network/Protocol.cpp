#include "network/Protocol.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace protocol {

std::optional<Command> parse_command(const std::string& text) {
  auto parsed = json::parse(text, nullptr, false);
  if (parsed.is_discarded()) return std::nullopt;
  Command cmd;
  cmd.action = parsed.value("action", "");
  if (cmd.action == "request_layers" || cmd.action == "layers") {
    cmd.type = CommandType::RequestLayers;
  } else {
    cmd.type = CommandType::Unknown;
  }
  return cmd;
}

std::string encode_layers(const std::vector<std::string>& layers) {
  json payload;
  payload["cmd"] = "SCENE_LAYERS";
  payload["layers"] = layers;
  return payload.dump();
}

std::string encode_mesh_header(std::size_t vertex_count,
                               std::size_t face_count) {
  json payload;
  payload["cmd"] = "SCENE_UPDATE";
  payload["path"] = "/World/TestMesh";
  payload["type"] = "mesh";
  payload["action"] = "full_update";
  payload["vertex_count"] = vertex_count;
  payload["face_count"] = face_count;
  payload["components"] = 3;
  return payload.dump();
}

std::string encode_mesh_payload(const MeshPayload& mesh) {
  json payload;
  payload["points"] = mesh.points;
  payload["counts"] = mesh.counts;
  payload["indices"] = mesh.indices;
  payload["colors"] = mesh.colors;
  return payload.dump();
}

std::string encode_ack(const std::string& action) {
  json payload;
  payload["cmd"] = "UI_ACK";
  payload["action"] = action;
  return payload.dump();
}

std::string encode_error(const std::string& reason) {
  json payload;
  payload["cmd"] = "ERROR";
  payload["reason"] = reason;
  return payload.dump();
}

}  // namespace protocol
