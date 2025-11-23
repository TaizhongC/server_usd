## web_usd Architecture (Prototype)

This document describes the intended structure for the prototype refactor. It keeps transport, protocol, and scene responsibilities separate so new USD prim types can be added without touching networking code.

### Goals
- Server: only transport (HTTP/WS), no business logic.
- Protocol: only message shapes and JSON/binary encoding/decoding.
- Scene: owns the USD stage, observed prim registry, and snapshots/deltas.
- UI: minimal; only scene layers panel for now.

### Modules
- `core/Scene.*`
  - Owns `pxr::UsdStage`.
  - Registers USD change listeners (`UsdNotice::ObjectsChanged`) for observed prims.
  - Holds an `ObservedPrim` registry keyed by `SdfPath` with attributes of interest and snapshot builders.
  - Exposes `poll_deltas()` returning dirty prim snapshots (generic, not mesh-specific).
  - Provides convenience accessors (e.g., `layers()`).
- `network/Server.*`
  - Mongoose HTTP/WS only: accept, serve static, upgrade, close, broadcast.
  - Parses incoming WS text as Protocol commands; dispatches via callbacks to Scene handlers.
  - Sends only messages/buffers created by Protocol encoders.
- `network/Protocol.*`
  - Defines typed commands from client (e.g., `RequestLayers`, `SubscribeScene`).
  - Encodes outbound events: `SceneUpdateHeader`, `SceneLayers`, `Ack`, `Error`.
  - Centralizes JSON field names; Server never hardcodes them.
- `ui/Layout.*`
  - Builds the minimal UI JSON for scene layers (optional: manual refresh button).

### Data model (snapshots)
- Base: `PrimSnapshot` (path, type token, time code).
- Derived examples:
  - `MeshSnapshot`: points, faceVertexCounts, faceVertexIndices, displayColor (normals/uvs later).
  - Future: `CurveSnapshot`, `VolumeSnapshot`, etc.
- `PrimDelta` (or `SceneDelta`) pairs dirty flags with a concrete snapshot instance.

### Observed prim registry
- `ObservedPrim { SdfPath path; TfToken type; std::unordered_set<TfToken> attrs; PendingFlags pending; SnapshotFn snapshot; }`
- Registration is per-prim (not per-type), so adding a new prim type is just adding another entry with its attrs and snapshot function.
- `PendingFlags` marks which attrs were touched since last poll.

### Change detection flow
1) `Scene` registers `TfNotice::Register(this, &Scene::on_objects_changed, stage_)`.
2) `on_objects_changed`:
   - Ignore notices from other stages.
   - For `ResyncedPaths`: find observed prims with that prefix -> mark all flags dirty.
   - For `ChangedInfoOnlyPaths`: extract prim path + attr token -> if in `attrs` for that prim, mark that flag.
3) `poll_deltas()`:
   - For each observed prim with pending flags, call its `snapshot` function to pull current USD data for the dirty attrs.
   - Return a vector of `PrimDelta`; clear flags.

### Server/protocol flow (minimal prototype)
- Inbound WS:
  - Text -> Protocol `parse_command` -> dispatch via callbacks to Scene handlers.
  - Respond with `Ack` or `Error`.
- Outbound WS:
  - Scene push: poll deltas; for each delta, Protocol builds `SceneUpdateHeader` (JSON) and binary payload (e.g., float array for points). Server sends header text + binary.
  - Layers request: Protocol builds `SceneLayers` from `Scene::layers()`.

### Extending for new prim types
- Add a new `ObservedPrim` entry:
  - Path (or register multiple paths).
  - Type token.
  - Attr tokens of interest.
  - Snapshot function that fills a derived `PrimSnapshot`.
- No changes to notice handler, Server, or existing protocol encoders required; only add new protocol encoder if the wire format differs.

### UI notes
- Keep only scene layers UI in the shipped layout.
- Optional: add a "refresh layers" control; keep other test controls out of the main layout.

### Minimal implementation checklist
- [ ] Scene: observed-prim registry, notice hook, poll_deltas, layers.
- [ ] Protocol: typed commands/events, encoders/decoders.
- [ ] Server: transport-only, callback-based dispatch, uses Protocol for all payloads.
- [ ] UI: scene-layers layout JSON.
