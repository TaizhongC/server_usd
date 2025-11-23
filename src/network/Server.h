#pragma once

#include <string>
#include <vector>

#include "mongoose.h"

#ifdef sleep
#undef sleep  // Avoid conflict with tbb/Windows Sleep usage in USD headers.
#endif

class Application;
class Layout;
class Scene;

struct ServerConfig {
  std::string host = "0.0.0.0";
  int port = 8000;
  std::string web_root;
};

class Server {
public:
  Server(ServerConfig config, Application& app, const Layout& layout, const Scene& scene);
  ~Server();

  bool start();
  void poll_once(int milliseconds);
  void stop();
  bool running() const { return running_; }

private:
  static void handle_event(mg_connection* c, int ev, void* ev_data);
  void handle_http(mg_connection* c, mg_http_message* hm);
  void handle_ws_open(mg_connection* c);
  void handle_ws_msg(mg_connection* c, mg_ws_message* msg);
  void handle_close(mg_connection* c);
  void send_frame(mg_connection* c);
  void send_ui_layout(mg_connection* c);
  void send_scene_layers(mg_connection* c);
  void broadcast_frame(const std::vector<float>& vertices);

  std::string websocket_address() const;

  ServerConfig config_;
  Application& app_;
  const Layout& layout_;
  const Scene& scene_;

  mg_mgr mgr_;
  mg_connection* listener_;
  std::vector<mg_connection*> clients_;
  bool running_;
};
