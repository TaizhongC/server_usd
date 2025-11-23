#include "network/Server.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <string>
#include <vector>

#include "core/Scene.h"
#include "network/Protocol.h"
#include "ui/Layout.h"

namespace {
void log_crash(const std::string& msg) {
  try {
    std::ofstream out("crash.log", std::ios::app);
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    out << std::put_time(std::localtime(&t), "%F %T") << " " << msg
        << std::endl;
  } catch (...) {
    // Best effort only.
  }
}

uint16_t ntohs_u16(uint16_t v) { return static_cast<uint16_t>((v >> 8) | (v << 8)); }

std::string addr_to_string(const mg_addr& addr) {
  if (!addr.is_ip6) {
    const uint8_t* b = addr.ip;
    const uint16_t port = ntohs_u16(addr.port);
    std::ostringstream oss;
    oss << static_cast<unsigned>(b[0]) << "." << static_cast<unsigned>(b[1]) << "."
        << static_cast<unsigned>(b[2]) << "." << static_cast<unsigned>(b[3])
        << ":" << port;
    return oss.str();
  }
  return "[ipv6]";
}
}  // namespace

namespace {
std::vector<float> triangulate_positions(const MeshSnapshot& mesh) {
  std::vector<float> out;
  const auto& pts = mesh.points;
  const auto& counts = mesh.faceVertexCounts;
  const auto& indices = mesh.faceVertexIndices;
  std::size_t cursor = 0;
  for (std::size_t face = 0; face < counts.size(); ++face) {
    const int n = counts[face];
    if (n < 3) {
      cursor += static_cast<std::size_t>(n);
      continue;
    }
    for (int i = 1; i + 1 < n; ++i) {
      const int i0 = indices[cursor + 0];
      const int i1 = indices[cursor + i];
      const int i2 = indices[cursor + i + 1];
      const int ids[3] = {i0, i1, i2};
      for (int id : ids) {
        const std::size_t base = static_cast<std::size_t>(id) * 3;
        if (base + 2 >= pts.size()) continue;
        out.push_back(pts[base + 0]);
        out.push_back(pts[base + 1]);
        out.push_back(pts[base + 2]);
      }
    }
    cursor += static_cast<std::size_t>(n);
  }
  return out;
}
}  // namespace

Server::Server(ServerConfig config, const Layout& layout, const Scene& scene)
    : config_(std::move(config)),
      layout_(layout),
      scene_(scene),
      listener_(nullptr),
      running_(false) {
  mg_mgr_init(&mgr_);
}

Server::~Server() { stop(); }

bool Server::start() {
  if (running_) return true;

  // Reduce mongoose logging noise: errors only.
  mg_log_set(MG_LL_ERROR);

  const std::string address = websocket_address();
  listener_ =
      mg_http_listen(&mgr_, address.c_str(), &Server::handle_event, this);
  if (listener_ == nullptr) {
    std::cerr << "Failed to start listener on " << address << "\n";
    return false;
  }
  listener_->fn_data = this;

  running_ = true;
  std::cout << "HTTP/WebSocket listening on " << address << "\n";
  std::cout << "Serving static files from: " << config_.web_root << "\n";
  return true;
}

void Server::poll_once(int milliseconds) {
  if (!running_) return;
  mg_mgr_poll(&mgr_, milliseconds);
}

void Server::stop() {
  if (!running_) return;
  running_ = false;
  mg_mgr_free(&mgr_);
  listener_ = nullptr;
  clients_.clear();
}

void Server::handle_event(mg_connection* c, int ev, void* ev_data) {
  auto* self = static_cast<Server*>(c->fn_data);
  if (self == nullptr) {
    // Listener may already own fn_data; propagate to accepted connections.
    return;
  }
  switch (ev) {
    case MG_EV_HTTP_MSG:
      self->handle_http(c, static_cast<mg_http_message*>(ev_data));
      break;
    case MG_EV_WS_OPEN:
      self->handle_ws_open(c);
      break;
    case MG_EV_WS_MSG:
      self->handle_ws_msg(c, static_cast<mg_ws_message*>(ev_data));
      break;
    case MG_EV_CLOSE:
      self->handle_ws_close(c);
      break;
    default:
      break;
  }
}

void Server::handle_http(mg_connection* c, mg_http_message* hm) {
  if (mg_strcmp(hm->uri, mg_str("/ws")) == 0) {
    mg_ws_upgrade(c, hm, nullptr);
    return;
  }

  mg_http_serve_opts opts{};
  opts.root_dir = config_.web_root.c_str();
  mg_http_serve_dir(c, hm, &opts);
}

void Server::handle_ws_open(mg_connection* c) {
  {
    std::ostringstream oss;
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    const std::string addr = c ? addr_to_string(c->rem) : "unknown";
    oss << "[WS] " << std::put_time(std::localtime(&t), "%F %T")
        << " client connected from " << addr;
    std::cout << oss.str() << std::endl;
    log_crash(oss.str());
  }
  clients_.push_back(c);
  send_ui_layout(c);
  // Send stage metadata once.
  const StageInfo stage_info = scene_.stage_info();
  const std::string stage_msg =
      protocol::encode_stage_info(stage_info.metersPerUnit, stage_info.upAxis);
  mg_ws_send(c, stage_msg.c_str(), stage_msg.size(), WEBSOCKET_OP_TEXT);

  send_scene_layers(c);
  // Send the demo mesh on connect for quick testing.
  const auto snapshot = scene_.mesh_snapshot();
  const auto verts = triangulate_positions(snapshot);
  const auto header =
      protocol::encode_mesh_header(verts.size() / 3,
                                   snapshot.faceVertexCounts.size());
  mg_ws_send(c, header.c_str(), header.size(), WEBSOCKET_OP_TEXT);
  mg_ws_send(c,
             reinterpret_cast<const char*>(verts.data()),
             verts.size() * sizeof(float),
             WEBSOCKET_OP_BINARY);
}

void Server::handle_ws_close(mg_connection* c) {
  {
    std::ostringstream oss;
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    const std::string addr = c ? addr_to_string(c->rem) : "unknown";
    oss << "[WS] " << std::put_time(std::localtime(&t), "%F %T")
        << " client disconnected from " << addr;
    std::cout << oss.str() << std::endl;
    log_crash(oss.str());
  }
  clients_.erase(
      std::remove(clients_.begin(), clients_.end(), c),
      clients_.end());
}

void Server::handle_ws_msg(mg_connection* c, mg_ws_message* msg) {
  try {
    std::string payload(msg->data.buf, msg->data.len);
    log_crash(std::string("WS msg: ") + payload);
    auto cmd = protocol::parse_command(payload);
    if (!cmd.has_value()) return;

    switch (cmd->type) {
      case protocol::CommandType::RequestLayers:
        send_scene_layers(c);
        {
          const std::string ack = protocol::encode_ack(cmd->action);
          mg_ws_send(c, ack.c_str(), ack.size(), WEBSOCKET_OP_TEXT);
        }
        break;
      case protocol::CommandType::Unknown: {
        const std::string text = protocol::encode_error("unknown_action");
        mg_ws_send(c, text.c_str(), text.size(), WEBSOCKET_OP_TEXT);
        break;
      }
    }
  } catch (const std::exception& e) {
    log_crash(std::string("WS msg exception: ") + e.what());
  } catch (...) {
    log_crash("WS msg unknown exception");
  }
}

void Server::send_ui_layout(mg_connection* c) {
  const std::string layout = layout_.to_json();
  if (c && !c->is_closing) {
    mg_ws_send(c, layout.c_str(), layout.size(), WEBSOCKET_OP_TEXT);
  }
}

void Server::send_scene_layers(mg_connection* c) {
  if (!c || c->is_closing) return;
  const std::string text = protocol::encode_layers(scene_.layers());
  mg_ws_send(c, text.c_str(), text.size(), WEBSOCKET_OP_TEXT);
}

std::string Server::websocket_address() const {
  return "http://" + config_.host + ":" + std::to_string(config_.port);
}
