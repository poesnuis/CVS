# Color Vision Simulator (CVS)

C++20 Windows x64 desktop color-vision simulation using Windows Graphics Capture,
Direct3D 11 and an embedded HLSL shader. Native Win32 controls; no Python, Node,
Visual Studio or runtime dependency installation is needed to use the packaged exe.

**Development build — see [verified results](docs/TESTING.md). A successful build
or shader test does not establish desktop or game compatibility.**

Verified on a Windows CI display: real capture/exclusion/output and click-through,
actual application hotkeys/tray registration/preset controls/emergency disable,
17,703 CPU assertions and 69,120 WARP pixel comparisons. Physical multi-monitor,
Windows 10/11 hardware/game compatibility and hardware benchmarks remain open.

## Install and use

1. Download the `ColorVisionSimulator-win-x64` artifact from a successful
   [Windows build](https://github.com/poesnuis/CVS/actions/workflows/windows.yml).
2. Extract `ColorVisionSimulator-win-x64.zip` and run
   `ColorVisionSimulator/ColorVisionSimulator.exe` as an ordinary user.
3. Select a preset, severity, and one or more displays, then enable filtering.
   Filtering always starts **off**, including after an unexpected exit.
4. Use Advanced for cone/custom matrix experiments and separate artistic controls.
   Presets saves, loads, imports and exports JSON. Settings reports shortcut conflicts.
5. Close hides the UI to the tray by default. Use tray → Exit safely to stop the
   application, or disable this preference in Settings.

Requires Windows 10 **2004 / build 19041 or later**, or Windows 11, x64, D3D11
feature level 11.0 hardware, and an SDR display. HDR-enabled selected outputs
are rejected. The application does not request administrator rights.

**Emergency disable: Ctrl+Alt+Home.** A separate key-polling thread also checks
this chord. `ColorVisionSimulator.exe --disable` tells an existing instance to
stop filtering. Terminating the process removes its windows. `--safe-mode` ignores
saved settings for that launch. No emergency mechanism can guarantee recovery
from an operating-system or graphics-driver hang.

## Presets and science

Protanopia, protanomaly, deuteranopia, deuteranomaly, tritanopia and tritanomaly;
achromatopsia, achromatomaly, blue-cone/cone monochromacy; acquired red-green and
blue-yellow proxies; generalized desaturation; custom LMS gains and custom RGB matrix.

Machado's published 11-step severity tables are interpolated in linear sRGB.
Brettel's two-plane projection uses Smith-Pokorny LMS and a white-neutral sRGB
adaptation. Recommended mode uses Machado for protan/deutan and Brettel for tritan.
Partial Brettel severity is a blend. Rare/acquired/monochromacy presets and
tritanomaly are explicitly experimental appearance approximations. They do not
simulate all physiology, acuity, photophobia or individual experience. This is an
appearance simulation, not a diagnostic tool. See [science and sources](docs/SCIENCE.md).

Scientific severity is separate from artistic saturation, brightness in EV and
contrast (linear 18% pivot). Cone gains apply only to Custom LMS; the surviving
cone selector applies only to Cone monochromacy; matrix entries apply only to
Custom RGB matrix. The preview and GPU use the same transform parameters.

## Shortcuts

| Default | Action |
|---|---|
| Ctrl+Alt+F8 | Toggle |
| Ctrl+Alt+F9 | Next preset |
| Ctrl+Alt+F10 | Previous preset |
| Ctrl+Alt+F11 | Severity +5% |
| Ctrl+Alt+F12 | Severity -5% (registration may fail: F12 is reserved by Windows) |
| Ctrl+Alt+Home | Fixed emergency disable |

Settings allows custom combinations and detects failed registrations and duplicate
bindings. For the F12 action, choose e.g. Ctrl+Alt+PageDown. New custom bindings
are rolled back if any registration fails. Reserved keys are not intercepted with hooks.

## Rendering, fullscreen and diagnostics

Desktop overlay mode targets the composited desktop and compatible borderless
fullscreen applications, including ordinary video. Selected displays have independent
capture sessions/devices on their adapters, at native resolution. GPU textures stay on
the GPU; no per-frame CPU readback occurs. Overlay windows are excluded from capture,
do not accept input, and appear only after a frame is rendered.

Exclusive fullscreen can bypass desktop composition and is **not supported by the
desktop overlay**. Optional ReShade integration is a separate, manually installed
shader for permitted games. CVS does not install or inject ReShade. See
[game-shader instructions](optional-integrations/game-shaders/README.md).
Protected/DRM video, secure desktop, lock screen and games forbidding overlays or
ReShade are unsupported. Capture borders, hardware cursor and some higher-z-order
system windows may remain unfiltered. Disable other color filters when comparing.

Settings displays observed frame rate, asynchronous GPU draw timing, capture-to-submit
frame age and process private memory. Frame age is **not end-to-end input/display
latency**. GPU timing excludes capture/composition/presentation. 60 FPS is a target,
not a benchmark claim; static desktops may supply fewer new frames.

Display changes rebuild sessions; device/capture exceptions hide overlays and retry
at most three times. Persistent faults disable filtering. Settings/logs are under
`%LOCALAPPDATA%\ColorVisionSimulator`. Invalid settings are preserved, with recovered
settings saved separately. Diagnostics are bounded and contain no captured images.

## Build and test

Windows: Visual Studio 2022 C++ workload, Windows SDK 10.0.22621 or newer, CMake 3.24+.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix dist/ColorVisionSimulator
```

MSVC uses `/MT` (static C++ runtime). Shader source is embedded and compiled using
Windows' system D3DCompiler at runtime. No files need to accompany the exe for it to
launch, though the distribution includes documentation and license notices.

Linux/macOS can build and test the portable scientific/configuration library with
`cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`.
The Windows UI/capture binary is built only on Windows. CI uploads the executable
package, matching source archive and test logs, and does not publish public releases.

## Development memory

[Notion development headquarters](https://www.notion.so/4198721d702e8132a66ad28c995aa2d8)
contains the roadmap, research, decisions, bugs, journal, testing and handoff.
[Current handoff](docs/HANDOFF.md), [architecture](docs/ARCHITECTURE.md) and
[session instructions](AGENTS.md) are repository mirrors. Code and test evidence
take precedence over status labels.

Original code: MIT. Dependencies/data notices are in [LICENSES](LICENSES/README.md).
ShaderGlass was evaluated as an architectural reference; its code is not included.
