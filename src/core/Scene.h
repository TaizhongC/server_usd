#pragma once

#include <string>
#include <vector>

class Application;

class Scene {
public:
  explicit Scene(Application& app) : app_(app) {}
  std::vector<std::string> layers() const;

private:
  Application& app_;
};
