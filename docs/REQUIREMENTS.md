# Requirements and acceptance gates

Source: the user's Color Vision Simulator development specification. This file is
the durable summary; the corresponding Notion roadmap tracks evidence and status.
The deliverable must be a real Windows x64 executable, with source and instructions,
not merely a plan, mockup or source-only prototype.

## Phase acceptance

| Phase | Acceptance criteria |
|---|---|
| 1 Research | Connect Notion, inspect existing repo/project, compare WGC/DDA/ShaderGlass/ReShade, cite scientific models, record decisions and specific tasks |
| 2 Rendering | GPU capture and simulation across selected desktop; native-resolution output, interaction passthrough, recursion prevention, safe enable/disable, app switching |
| 3 Science | Required presets, correct sRGB/LMS, established models where applicable, severity, custom cones/matrix, independent references, honest experimental labels |
| 4 UI | Search, severity, enable, monitor/mode selection, preview, advanced controls, JSON preset manager, tray, configurable/conflict-checked shortcuts, settings/diagnostics |
| 5 Fullscreen | Borderless behavior tested; exclusive limitation documented; optional user-managed game shader where permitted; no bypass/injection/kernel modifications |
| 6 Reliability | Multi-monitor, mixed DPI/refresh/adapters, display changes and device recovery tested; real diagnostics; hardware performance measured where available |
| 7 Packaging | MSVC x64 Release build, self-contained exe, embedded shaders, licenses/examples/source, reproducible build, automated tests and genuine downloadable artifact |
| 8 Documentation | Dashboard, roadmap, research, decisions, bugs, tests, journal and handoff reconciled; AGENTS.md enables continuation |

## Required simulation coverage

Protanopia, protanomaly, deuteranopia, deuteranomaly, tritanopia, tritanomaly,
achromatopsia, achromatomaly, blue-cone and cone monochromacy, acquired red-green
and blue-yellow impairment proxies, generalized desaturation and custom L/M/S.
CVS also includes a custom linear RGB matrix preset.

Scientific 0–100% severity is distinct from artistic saturation, brightness and
contrast. Imported/exported settings must round-trip and invalid values must fail
before replacing active settings. Rare/acquired conditions must remain visibly
experimental; no claim that these reproduce an individual's physiology.

## Definition of done and remaining gates

| Requirement | State/evidence boundary |
|---|---|
| Genuine standalone x64 exe | Built with MSVC /MT; Windows system imports; executable-only launch test |
| Desktop-wide filtering | Real WGC and overlay tested on one 1024×768 Windows runner display |
| Scientific presets and severity | CPU/reference/WARP coverage; experimental limitations documented |
| Preset persistence/import/export | JSON and atomic persistence tests; GUI save/load tested; file-picker interaction not manually tested |
| Global shortcuts and tray | Real injected hotkeys, registration, conflict rollback and emergency tests; full tray-menu exploration remains manual |
| Safe disable and process cleanup | Normal re-enable/disable and emergency tested; forced crash and driver hang manual tests remain |
| Multiple monitors | Implemented per-display device/capture; physical mixed-monitor matrix unverified |
| Borderless games | Intended composited path, no game tested yet |
| Exclusive fullscreen | Overlay unsupported; supplied ReShade integration unverified |
| Performance | Diagnostics implemented; 1080p/1440p/4K and 60 FPS hardware benchmarks unavailable |
| Tests/source/build docs | Included; latest actual run in TESTING.md |
| Notion and handoff | Persistent workspace and local mirrors; see HANDOFF.md for current checkpoint |

All unverified acceptance gates remain open even when a build or narrower test passes.
No public release or merge is implied by a downloadable development artifact.
