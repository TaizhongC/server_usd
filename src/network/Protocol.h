#pragma once

#include <cstddef>
#include <string>

namespace protocol {
std::string build_scene_update(std::size_t vertex_count);
std::string build_ack(const std::string& action);
}  // namespace protocol
