#include "core/Application.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <functional>
#include <iterator>

#ifdef HAS_USD_LITE
#include <pxr/base/vt/array.h>
#include <pxr/usd/usdGeom/cube.h>
#endif

namespace {
void log_crash(const std::string& msg) {
  try {
    std::ofstream out("crash.log", std::ios::app);
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    out << std::put_time(std::localtime(&t), "%F %T") << " " << msg
        << std::endl;
  } catch (...) {
    // Best effort only.
  }
}
}  // namespace

Application::Application() : phase_(0.0f), speed_(1.0f)
#ifdef HAS_USD_LITE
                 , usd_enabled_(true)
#endif
{
#ifdef HAS_USD_LITE
  // Default: USD enabled; allow opting out with ZSPACE_DISABLE_USD=1.
  if (const char* env = std::getenv("ZSPACE_DISABLE_USD")) {
    std::string v(env);
    if (v == "1" || v == "true" || v == "yes") {
      usd_enabled_ = false;
      std::cerr << "[Application] USD disabled via ZSPACE_DISABLE_USD\n";
      log_crash("USD disabled via ZSPACE_DISABLE_USD");
    }
  }

  if (usd_enabled_) {
    auto load_required = [](const char* name) -> bool {
      HMODULE mod = LoadLibraryA(name);
      if (!mod) {
        DWORD err = GetLastError();
        std::cerr << "[Application] LoadLibrary failed for " << name
                  << " (err=" << err << ")\n";
        std::ostringstream oss;
        oss << "LoadLibrary failed for " << name << " err=" << err;
        log_crash(oss.str());
        return false;
      }
      return true;
    };
    if (!load_required("usd_ms.dll") ||
        !load_required("tbb.dll") ||
        !load_required("tbbmalloc.dll")) {
      usd_enabled_ = false;
      std::cerr << "[Application] Disabling USD due to missing DLLs\n";
      log_crash("Disabling USD due to missing dlls");
    } else {
      std::cerr << "[Application] USD enabled by default (clear with ZSPACE_DISABLE_USD)\n";
      log_crash("USD enabled by default");
    }
  } else {
    log_crash("USD disabled by env");
  }
#endif
}

std::vector<float> Application::generate_frame() {
  std::vector<float> verts;

#ifdef HAS_USD_LITE
  if (usd_enabled_) {
    // Ensure an empty stage exists; skip mesh writes to avoid instability.
    ensure_stage();
  }
#endif

  // Emit the authored cube (12 triangles) and triangle mesh (1 triangle).
  const float s = 0.5f;
  // Cube vertices, 12 tris * 3 verts each * 3 components
  const float cube[] = {
      // back face
      -s, -s, -s,  s, -s, -s,  s,  s, -s,
      -s, -s, -s,  s,  s, -s, -s,  s, -s,
      // front face
      -s, -s,  s,  s, -s,  s,  s,  s,  s,
      -s, -s,  s,  s,  s,  s, -s,  s,  s,
      // left face
      -s, -s, -s, -s,  s, -s, -s,  s,  s,
      -s, -s, -s, -s,  s,  s, -s, -s,  s,
      // right face
       s, -s, -s,  s,  s, -s,  s,  s,  s,
       s, -s, -s,  s,  s,  s,  s, -s,  s,
      // top face
      -s,  s, -s,  s,  s, -s,  s,  s,  s,
      -s,  s, -s,  s,  s,  s, -s,  s,  s,
      // bottom face
      -s, -s, -s,  s, -s, -s,  s, -s,  s,
      -s, -s, -s,  s, -s,  s, -s, -s,  s,
  };
  verts.insert(verts.end(), std::begin(cube), std::end(cube));

  const float tri[] = {
      -0.4f, 0.0f, 0.0f,
       0.4f, 0.0f, 0.0f,
       0.0f, 0.7f, 0.0f,
  };
  verts.insert(verts.end(), std::begin(tri), std::end(tri));

  last_frame_ = verts;
  return verts;
}

void Application::set_speed(float s) {
  speed_ = std::clamp(s, 0.05f, 10.0f);
}
#ifdef HAS_USD_LITE
bool Application::ensure_stage() {
  if (stage_) return true;
  log_crash("ensure_stage start");
  try {
    stage_ = pxr::UsdStage::CreateInMemory();
    if (!stage_) {
      std::cerr << "[Application] Failed to create USD stage\n";
      log_crash("USD stage creation failed");
      return false;
    }
    // Define a root Xform so the stage is valid but empty.
    stage_->DefinePrim(pxr::SdfPath("/World"), pxr::TfToken("Xform"));
    stage_->SetDefaultPrim(stage_->GetPrimAtPath(pxr::SdfPath("/World")));

    // Author a cube and triangle under /World for quick graph inspection.
    pxr::UsdPrim world = stage_->GetPrimAtPath(pxr::SdfPath("/World"));
    if (world) {
      pxr::UsdGeomCube cube =
          pxr::UsdGeomCube::Define(stage_, pxr::SdfPath("/World/Cube"));
      if (cube) {
        cube.CreateSizeAttr().Set(1.0);  // size is a double attr
        pxr::VtArray<pxr::GfVec3f> color(1);
        color[0] = pxr::GfVec3f(0.2f, 0.6f, 1.0f);
        cube.CreateDisplayColorAttr().Set(color);
      }

      pxr::UsdGeomMesh tri =
          pxr::UsdGeomMesh::Define(stage_, pxr::SdfPath("/World/Triangle"));
      if (tri) {
        pxr::VtArray<pxr::GfVec3f> points(3);
        points[0] = pxr::GfVec3f(-0.5f, 0.0f, 0.0f);
        points[1] = pxr::GfVec3f(0.5f, 0.0f, 0.0f);
        points[2] = pxr::GfVec3f(0.0f, 0.7f, 0.0f);
        tri.CreatePointsAttr().Set(points);

        pxr::VtArray<int> counts(1);
        counts[0] = 3;
        tri.CreateFaceVertexCountsAttr().Set(counts);

        pxr::VtArray<int> indices(3);
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        tri.CreateFaceVertexIndicesAttr().Set(indices);

        pxr::VtArray<pxr::GfVec3f> tri_color(1);
        tri_color[0] = pxr::GfVec3f(1.0f, 0.4f, 0.2f);
        tri.CreateDisplayColorAttr().Set(tri_color);
      }
    }

    std::cerr << "[Application] Empty USD stage initialized in-memory with /World\n";
    log_crash("USD empty stage initialized with /World");
    return true;
  } catch (const std::exception& e) {
    std::cerr << "[Application] USD stage init error: " << e.what() << "\n";
    log_crash(std::string("USD stage init error: ") + e.what());
    usd_enabled_ = false;
    stage_.Reset();
    return false;
  } catch (...) {
    std::cerr << "[Application] USD stage init unknown error\n";
    log_crash("USD stage init unknown error");
    usd_enabled_ = false;
    stage_.Reset();
    return false;
  }
}

void Application::write_to_usd(const std::vector<float>& verts) {
  (void)verts;
  // No-op while USD runtime is stubbed.
}

std::vector<std::string> Application::stage_layers() const {
  std::vector<std::string> layers;
  if (!usd_enabled_ || !stage_) return layers;
  try {
    const pxr::UsdPrim default_prim = stage_->GetDefaultPrim();
    if (default_prim) {
      std::function<void(const pxr::UsdPrim&, int)> walk =
          [&](const pxr::UsdPrim& prim, int depth) {
            std::string line(depth * 2, ' ');
            line += prim.GetPath().GetString();
            const std::string type = prim.GetTypeName().GetString();
            if (!type.empty()) {
              line += " (" + type + ")";
            }
            layers.push_back(std::move(line));
            for (const auto& child : prim.GetChildren()) {
              walk(child, depth + 1);
            }
          };
      walk(default_prim, 0);
    }
  } catch (...) {
    // Ignore errors; return what we have.
  }
  return layers;
}
#endif
