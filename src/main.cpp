#include <atomic>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "core/Application.h"
#include "core/Scene.h"
#include "network/Server.h"
#include "ui/Layout.h"

namespace {
std::atomic_bool g_running{true};

void handle_signal(int) { g_running.store(false); }

void log_line(const std::string& msg) {
  try {
    std::ofstream out("crash.log", std::ios::app);
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    out << std::put_time(std::localtime(&t), "%F %T") << " " << msg
        << std::endl;
  } catch (...) {
    // best effort
  }
}
}  // namespace

int main() {
  log_line("zspace_web start");
  std::signal(SIGINT, handle_signal);
  std::signal(SIGTERM, handle_signal);

  Application app;
  Layout layout;
  Scene scene(app);

  ServerConfig config;
  config.host = "0.0.0.0";
  config.port = 8000;
#ifdef WEB_ROOT_DIR
  config.web_root = WEB_ROOT_DIR;
#else
  config.web_root = "web";
#endif

  std::cout << "Starting server at http://" << config.host << ":"
            << config.port << std::endl;
  log_line("Application constructed");

  Server server(config, app, layout, scene);
  if (!server.start()) {
    std::cerr << "Unable to start server\n";
    log_line("Server start failed");
    return 1;
  }
  log_line("Server started");

  std::cout << "Press Ctrl+C to stop." << std::endl;
  while (g_running.load()) {
    server.poll_once(50);
  }

  server.stop();
  return 0;
}
