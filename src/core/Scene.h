#pragma once

#include <string>
#include <vector>

class Scene {
public:
  Scene() = default;
  std::vector<std::string> layers() const;
};
