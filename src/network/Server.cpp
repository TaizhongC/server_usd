#include "network/Server.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/Application.h"
#include "core/Scene.h"
#include "network/Protocol.h"
#include "ui/Layout.h"

using json = nlohmann::json;

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
}  // namespace

Server::Server(ServerConfig config, Application& app, const Layout& layout, const Scene& scene)
    : config_(std::move(config)),
      app_(app),
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
      self->handle_close(c);
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
  log_crash("WS open");
  clients_.push_back(c);
  send_ui_layout(c);
  send_frame(c);
  send_scene_layers(c);
}

void Server::handle_ws_msg(mg_connection* c, mg_ws_message* msg) {
  try {
    std::string payload(msg->data.buf, msg->data.len);
    log_crash(std::string("WS msg: ") + payload);
    auto parsed = json::parse(payload, nullptr, false);
    if (parsed.is_discarded()) return;

    const std::string action = parsed.value("action", "");

    if (action == "generate") {
      auto verts = app_.generate_frame();
      const std::string ack = protocol::build_ack(action);
      mg_ws_send(c, ack.c_str(), ack.size(), WEBSOCKET_OP_TEXT);
      broadcast_frame(verts);
    } else if (action == "speed") {
      const double v = parsed.value("value", 1.0);
      app_.set_speed(static_cast<float>(v));
      auto verts = app_.generate_frame();  // Immediately reflect new speed.
      const std::string ack = protocol::build_ack(action);
      mg_ws_send(c, ack.c_str(), ack.size(), WEBSOCKET_OP_TEXT);
      broadcast_frame(verts);
    } else {
      const std::string ack = protocol::build_ack("unknown_action");
      mg_ws_send(c, ack.c_str(), ack.size(), WEBSOCKET_OP_TEXT);
    }
  } catch (const std::exception& e) {
    log_crash(std::string("WS msg exception: ") + e.what());
  } catch (...) {
    log_crash("WS msg unknown exception");
  }
}

void Server::handle_close(mg_connection* c) {
  log_crash("WS close");
  clients_.erase(
      std::remove(clients_.begin(), clients_.end(), c),
      clients_.end());
}

void Server::send_frame(mg_connection* c) {
  log_crash("send_frame begin");
  std::vector<float> verts =
      app_.last_frame().empty() ? app_.generate_frame()
                                   : app_.last_frame();

  const std::string header = protocol::build_scene_update(verts.size() / 3);
  if (c && !c->is_closing) {
    log_crash("send_frame header send");
    mg_ws_send(c, header.c_str(), header.size(), WEBSOCKET_OP_TEXT);
    log_crash("send_frame binary send");
    mg_ws_send(c,
               reinterpret_cast<const char*>(verts.data()),
               verts.size() * sizeof(float),
               WEBSOCKET_OP_BINARY);
  }
  log_crash("send_frame end");
}

void Server::broadcast_frame(const std::vector<float>& vertices) {
  log_crash("broadcast_frame begin: clients=" + std::to_string(clients_.size()));
  const std::string header = protocol::build_scene_update(vertices.size() / 3);
  for (auto* client : clients_) {
    if (client == nullptr || client->is_closing) continue;
    log_crash("broadcast_frame header send");
    mg_ws_send(client, header.c_str(), header.size(), WEBSOCKET_OP_TEXT);
    log_crash("broadcast_frame binary send");
    mg_ws_send(client,
               reinterpret_cast<const char*>(vertices.data()),
               vertices.size() * sizeof(float),
               WEBSOCKET_OP_BINARY);
  }
  log_crash("broadcast_frame end");
}

void Server::send_ui_layout(mg_connection* c) {
  const std::string layout = layout_.to_json();
  if (c && !c->is_closing) {
    mg_ws_send(c, layout.c_str(), layout.size(), WEBSOCKET_OP_TEXT);
  }
}

void Server::send_scene_layers(mg_connection* c) {
  if (!c || c->is_closing) return;
  json payload;
  payload["cmd"] = "SCENE_LAYERS";
  payload["layers"] = scene_.layers();
  const std::string text = payload.dump();
  mg_ws_send(c, text.c_str(), text.size(), WEBSOCKET_OP_TEXT);
}

std::string Server::websocket_address() const {
  return "http://" + config_.host + ":" + std::to_string(config_.port);
}
