#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <pxr/base/tf/hash.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>

struct PrimDelta {
  pxr::SdfPath path;
  pxr::TfToken type;
  bool resynced = false;
  std::vector<pxr::TfToken> attrs_changed;
};

struct StageInfo {
  double metersPerUnit = 1.0;
  std::string upAxis = "Z";
};

struct MeshSnapshot {
  std::vector<float> points;           // flattened xyz
  std::vector<int> faceVertexCounts;   // polygon counts
  std::vector<int> faceVertexIndices;  // indices into points
  std::vector<float> displayColor;     // flattened rgb per-face or per-vertex
};

class Scene {
public:
  Scene();
  std::vector<std::string> layers() const;

  void register_observed_prim(const pxr::UsdPrim& prim,
                              const std::vector<pxr::TfToken>& attrs);
  std::vector<PrimDelta> poll_deltas();
  MeshSnapshot mesh_snapshot() const;
  StageInfo stage_info() const;

private:
  struct TokenHash {
    size_t operator()(const pxr::TfToken& t) const { return pxr::TfHash{}(t); }
  };
  struct PathHash {
    size_t operator()(const pxr::SdfPath& p) const { return pxr::SdfPath::Hash()(p); }
  };
  struct ObservedPrim {
    pxr::SdfPath path;
    pxr::TfToken type;
    std::unordered_set<pxr::TfToken, TokenHash> attrs;
    bool resynced = false;
    std::unordered_set<pxr::TfToken, TokenHash> dirty_attrs;
  };

  pxr::UsdStageRefPtr stage_;
  std::unordered_map<pxr::SdfPath, ObservedPrim, PathHash> observed_;
};
