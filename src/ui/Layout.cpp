#include "ui/Layout.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string Layout::to_json() const {
  json ui;
  ui["cmd"] = "UI_BUILD";
  ui["controls"] = {
      {{"type", "button"}, {"label", "Generate Mesh"}, {"action", "generate"}},
      {{"type", "slider"},
       {"label", "Wave Speed"},
       {"action", "speed"},
       {"min", 0.1},
       {"max", 2.0},
       {"step", 0.1},
       {"value", 1.0}}};
  return ui.dump();
}
