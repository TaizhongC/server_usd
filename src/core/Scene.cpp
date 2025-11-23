#include "core/Scene.h"

#include "core/Application.h"

std::vector<std::string> Scene::layers() const {
#ifdef HAS_USD_LITE
  return engine_.stage_layers();
#else
  return {};
#endif
}
