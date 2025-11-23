#pragma once

#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <windows.h>
#endif

#ifdef HAS_USD_LITE
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#endif

class Application {
public:
  Application();

  // Generates geometry for the current scene and stores it as the latest frame.
  std::vector<float> generate_frame();

  void set_speed(float s);
  float speed() const { return speed_; }
  const std::vector<float>& last_frame() const { return last_frame_; }
#ifdef HAS_USD_LITE
  std::vector<std::string> stage_layers() const;
#endif

private:
#ifdef HAS_USD_LITE
  bool ensure_stage();
  void write_to_usd(const std::vector<float>& verts);

  pxr::UsdStageRefPtr stage_;
  pxr::UsdGeomMesh mesh_;
#endif

  float phase_;
  float speed_;
  std::vector<float> last_frame_;
#ifdef HAS_USD_LITE
  bool usd_enabled_;
#endif
};
