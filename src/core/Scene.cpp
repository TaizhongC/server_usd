#include "core/Scene.h"

#include <algorithm>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/tokens.h>

Scene::Scene() {
  try {
    stage_ = pxr::UsdStage::CreateInMemory();
    if (stage_) {
      stage_->DefinePrim(pxr::SdfPath("/World"), pxr::TfToken("Xform"));
      stage_->SetDefaultPrim(stage_->GetPrimAtPath(pxr::SdfPath("/World")));
    }
  } catch (...) {
    stage_.Reset();
  }
}

std::vector<std::string> Scene::layers() const {
  std::vector<std::string> result;
  if (!stage_) {
    result.push_back("(no USD stage)");
    return result;
  }
  try {
    const pxr::UsdPrim default_prim = stage_->GetDefaultPrim();
    if (!default_prim) {
      result.push_back("(no default prim)");
      return result;
    }
    for (const pxr::UsdPrim& prim : stage_->Traverse()) {
      std::string line = prim.GetPath().GetString();
      const std::string type = prim.GetTypeName().GetString();
      if (!type.empty()) {
        line += " (" + type + ")";
      }
      result.push_back(std::move(line));
    }
  } catch (...) {
    // best effort
  }
  return result;
}

void Scene::register_observed_prim(const pxr::UsdPrim& prim,
                                   const std::vector<pxr::TfToken>& attrs) {
  if (!prim) return;
  if (!stage_) {
    stage_ = prim.GetStage();
  }
  ObservedPrim entry;
  entry.path = prim.GetPath();
  entry.type = prim.GetTypeName();
  entry.attrs.insert(attrs.begin(), attrs.end());
  observed_[entry.path] = std::move(entry);
}

std::vector<PrimDelta> Scene::poll_deltas() {
  // No notices wired yet; return empty.
  return {};
}
