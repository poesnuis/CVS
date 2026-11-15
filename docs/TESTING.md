# Test evidence and compatibility

Initial verified result: Linux x64 GCC 14.2 Release CTest science/configuration suite
passes. Windows MSVC, D3D11 WARP and GUI smoke tests are being run through GitHub Actions.
This file will be updated with the actual run and commit before handoff.

## Automated scope

- sRGB reference values and 256-value round trip.
- Published Machado primary-column endpoints, identity and severity interpolation.
- Brettel white preservation, projection idempotence and branch separation.
- All 15 presets, three model selections, multiple severities, a 5x5x5 color grid.
- JSON round trip, atomic replacement, invalid schema/preset/range/hotkey/size rejection.
- Windows: compile embedded HLSL; 69,120 WARP rendered pixels compared with CPU,
  including artistic settings, all presets/models and severities (270 images).
- Windows smoke test: create native GUI and enumerate monitors; attempt tray and hotkey
  registration; exit safely without capture. Does not verify desktop overlay behavior.

## Manual matrix — unverified unless a result is explicitly recorded

| Case | Procedure / acceptance |
|---|---|
| Desktop recursion | Enable on a moving test pattern, switch apps for 5 min; no recursive accumulation or black capture |
| Input | Click/type/drag through overlay; cursor remains responsive |
| Preview | Compare swatch values with GPU test vectors; UI excluded from capture |
| Presets / severity | Cycle tray/hotkeys, test 0%, 50%, 100%, import/export/save/restart |
| Conflicts | Reserve hotkey in another app; CVS reports it and keeps prior valid bindings |
| Safety | Ctrl+Alt+Home, --disable, tray Exit, task termination; no remaining overlay |
| Displays | Single/multiple monitors, mixed DPI/refresh/adapters, portrait, negative coordinates |
| Reconfiguration | Change resolution/scaling, disconnect/reconnect display, sleep/resume |
| Device loss | Trigger driver reset in a controlled test; hide, rebuild or disable cleanly |
| Windows | Windows 10 2004+ and Windows 11 on real D3D11 hardware |
| Video | Ordinary SDR video playback; protected/DRM content excluded from support |
| Borderless games | Record game/version/API/display mode, input and focus changes |
| Exclusive games | Desktop overlay unsupported; optional permitted ReShade path tested separately |
| HDR | Enabled HDR display refused before overlay activation |
| Performance | Record OS, GPU, driver, resolution, refresh, FPS cap, duration, FPS/GPU ms/frame age/memory |

## Benchmarks

No hardware performance benchmarks available. 1080p, 1440p and 4K/60 remain targets.
WARP numerical tests are software rasterization correctness tests, not performance
measurements. Frame age is not true input-to-photon latency. Static content may yield
few new WGC frames; benchmark with moving content and note that condition.
