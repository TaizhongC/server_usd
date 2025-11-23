#pragma once

#include <string>
#include <vector>

class Application;

class Scene {
public:
  explicit Scene(Application& engine) : engine_(engine) {}
  std::vector<std::string> layers() const;

private:
  Application& engine_;
};
