# Test evidence and compatibility

Verified code revision: `0bed223025bbbe502416cfef2610b3f1abb6fea3`.
[Successful Windows run 43748600145](https://github.com/poesnuis/CVS/actions/runs/43748600145),
2026-11-15. MSVC 19.44 x64 Release on GitHub's `windows-2022` runner, one 1024×768
display. Subsequent documentation-only packaging commits do not change this code;
the package's `SOURCE_COMMIT.txt` identifies its exact source revision.

| Check | Actual result |
|---|---|
| MSVC Release build | Pass; genuine x64 GUI PE executable |
| CPU science/configuration | 17,703 assertions pass on Windows and GCC 14.2 Linux |
| Independent model references | 96 DaltonLens vectors, 288 channel comparisons included above |
| D3D11 WARP | 69,120 rendered pixels pass; maximum normalized channel error 5.77653e-07 |
| Real WGC capture integration | 120 samples at 1024×768; source transitions remain unfiltered in capture, output matches transform; maximum UNORM channel error 0.00183153 |
| Click-through | One injected click delivered to underlying window while overlay is displayed |
| Actual application | Global F8 enable/disable, F9 preset switching, severity, preset save/load, tray registration, re-enable and Ctrl+Alt+Home pass |
| Hotkey conflicts | Chord reserved on another window rejected; previous bindings restored |
| Standalone launch | Exe launched from otherwise empty `CVS Portable` directory without companion resources |
| GUI | Native screenshot reviewed; readable controls and paired preview, vertical scroll available |
| Package | Portable zip, exe, matching source zip, licenses, examples and checksums uploaded |

This is correctness evidence on a Windows CI virtual machine, not clinical validation,
a physical GPU benchmark, or a Windows 10/11 hardware/game compatibility matrix.

## Automated scope

- sRGB reference values and 256-value round trip.
- Published Machado primary-column endpoints, identity and severity interpolation.
- Independent pinned DaltonLens vectors with matching Judd-Vos adaptation. Its
  fractional Machado interpolation has a scaling defect; fixtures use exact Machado
  table steps, while interpolation is checked independently between endpoints.
- Brettel white preservation, projection idempotence and branch separation.
- All 15 presets, three model selections, multiple severities, a 5x5x5 color grid.
- JSON round trip, atomic replacement, invalid schema/preset/range/hotkey/size/array-shape rejection.
- Windows: compile embedded HLSL; 69,120 WARP rendered pixels compared with CPU,
  including artistic settings, all presets/models and severities (270 images).
- Real desktop test uses test-only GPU readback of a known-color window beneath the
  overlay. Production rendering has no CPU pixel readback.
- Application automation uses injected keyboard input through actual registered
  global hotkeys, exercises UI handlers, creates a real conflicting registration,
  and verifies emergency shutdown. It does not manually explore every menu/setting.

## Remaining manual matrix

| Case | Procedure / acceptance |
|---|---|
| Desktop recursion | Short controlled test passed; switch real apps for 5 min and check accumulation/black capture |
| Input | One click passed; extend to typing, dragging, multiple apps and games |
| Preview | Compare swatch values with GPU test vectors; UI excluded from capture |
| Presets / severity | Cycle tray/hotkeys, test 0%, 50%, 100%, import/export/save/restart |
| Conflicts | Same-process/different-window conflict passed; extend to other apps and OS-reserved chords |
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

All defaults registered on the runner, including F12. Microsoft's documented F12
reservation means this cannot be assumed on every Windows configuration.

## Benchmarks

No hardware performance benchmarks available. 1080p, 1440p and 4K/60 remain targets.
WARP numerical tests are software rasterization correctness tests, not performance
measurements. Frame age is not true input-to-photon latency. Static content may yield
few new WGC frames; benchmark with moving content and note that condition.
