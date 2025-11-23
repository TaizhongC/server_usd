#include "ui/Layout.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string Layout::to_json() const {
  json ui;
  ui["cmd"] = "UI_BUILD";
  ui["controls"] = {
      {{"type", "button"},
       {"label", "Refresh Layers"},
       {"action", "request_layers"}}};
  return ui.dump();
}
