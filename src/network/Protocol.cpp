#include "network/Protocol.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace protocol {
std::string build_scene_update(std::size_t vertex_count) {
  json header;
  header["cmd"] = "SCENE_UPDATE";
  header["path"] = "/World/DemoMesh";
  header["type"] = "mesh";
  header["action"] = "update_verts";
  header["vertex_count"] = vertex_count;
  header["components"] = 3;
  return header.dump();
}

std::string build_ack(const std::string& action) {
  json ack;
  ack["cmd"] = "UI_ACK";
  ack["action"] = action;
  return ack.dump();
}
}  // namespace protocol
