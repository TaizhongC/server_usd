#include "core/Scene.h"

#include <algorithm>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/mesh.h>

Scene::Scene() {
  try {
    stage_ = pxr::UsdStage::CreateInMemory();
    if (stage_) {
      stage_->DefinePrim(pxr::SdfPath("/World"), pxr::TfToken("Xform"));
      stage_->SetDefaultPrim(stage_->GetPrimAtPath(pxr::SdfPath("/World")));

      pxr::UsdGeomMesh mesh =
          pxr::UsdGeomMesh::Define(stage_, pxr::SdfPath("/World/TestMesh"));
      if (mesh) {
        pxr::VtArray<pxr::GfVec3f> pts(4);
        pts[0] = pxr::GfVec3f(-0.5f, -0.5f, 0.0f);
        pts[1] = pxr::GfVec3f(0.5f, -0.5f, 0.0f);
        pts[2] = pxr::GfVec3f(0.5f, 0.5f, 0.0f);
        pts[3] = pxr::GfVec3f(-0.5f, 0.5f, 0.0f);
        mesh.CreatePointsAttr().Set(pts);

        pxr::VtArray<int> counts(1);
        counts[0] = 4;
        mesh.CreateFaceVertexCountsAttr().Set(counts);

        pxr::VtArray<int> indices(4);
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 3;
        mesh.CreateFaceVertexIndicesAttr().Set(indices);

        pxr::VtArray<pxr::GfVec3f> color(1);
        color[0] = pxr::GfVec3f(0.2f, 0.6f, 1.0f);
        mesh.CreateDisplayColorAttr().Set(color);
      }
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

MeshSnapshot Scene::mesh_snapshot() const {
  MeshSnapshot snapshot;
  if (!stage_) return snapshot;
  pxr::UsdPrim prim = stage_->GetPrimAtPath(pxr::SdfPath("/World/TestMesh"));
  if (!prim) return snapshot;
  pxr::UsdGeomMesh mesh(prim);
  pxr::VtArray<pxr::GfVec3f> pts;
  pxr::VtArray<int> counts;
  pxr::VtArray<int> indices;
  pxr::VtArray<pxr::GfVec3f> color;

  mesh.GetPointsAttr().Get(&pts);
  mesh.GetFaceVertexCountsAttr().Get(&counts);
  mesh.GetFaceVertexIndicesAttr().Get(&indices);
  mesh.GetDisplayColorAttr().Get(&color);

  snapshot.points.reserve(pts.size() * 3);
  for (const auto& p : pts) {
    snapshot.points.push_back(p[0]);
    snapshot.points.push_back(p[1]);
    snapshot.points.push_back(p[2]);
  }
  snapshot.faceVertexCounts.assign(counts.begin(), counts.end());
  snapshot.faceVertexIndices.assign(indices.begin(), indices.end());
  snapshot.displayColor.reserve(color.size() * 3);
  for (const auto& c : color) {
    snapshot.displayColor.push_back(c[0]);
    snapshot.displayColor.push_back(c[1]);
    snapshot.displayColor.push_back(c[2]);
  }
  return snapshot;
}
