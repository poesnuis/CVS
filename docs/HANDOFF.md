# Development Handoff — Current State

Updated: 2026-11-15 (America/New_York).
Repository: https://github.com/poesnuis/CVS
Branch: `codex/color-vision-simulator`; [draft PR #1](https://github.com/poesnuis/CVS/pull/1).
Verified implementation commit: `0bed223025bbbe502416cfef2610b3f1abb6fea3`.
Documentation-only commits may follow; inspect `git rev-parse HEAD` and git status.
The packaged `SOURCE_COMMIT.txt` records the exact checkout used for that artifact.

## Latest verified milestone and build

A genuine standalone MSVC x64 Release executable exists and launches from an isolated
folder containing only that exe. Original HLSL is embedded. Import inspection shows
Windows system DLLs and no dynamic MSVC runtime dependency.

[Verified Windows build](https://github.com/poesnuis/CVS/actions/runs/43748600145):
17,703 CPU assertions; 69,120 WARP pixels (maximum channel error 5.77653e-07);
real WGC capture/exclusion/output/parameter checks at 1024×768 (120 samples,
maximum UNORM error 0.00183153); one injected click through the overlay; actual app
F8/F9 shortcuts, severity, preset save/load, tray registration, hotkey-conflict rollback,
re-enable and Ctrl+Alt+Home. GUI screenshot reviewed. See TESTING.md for limits.

Use the newest successful build on this branch from
https://github.com/poesnuis/CVS/actions/workflows/windows.yml for the final documented
package. The artifact includes portable zip, matching source zip and checksums. It is
a development artifact; no public release or merge has been performed.

## Implemented components

- `src/core`: linear sRGB, Machado severity tables, Brettel/LMS, 15 presets,
  experimental labels, artistic/custom controls and strict atomic JSON configuration.
- `src/win`: WGC per-monitor/adapter capture, D3D11 renderer, native GUI/preview,
  preset manager, tray, configurable hotkeys, diagnostics, retries and safe disable.
- `shaders`: embedded desktop shader. `optional-integrations/game-shaders`: separate
  ReShade FX source/instructions (unverified in ReShade/games).
- `tests`: independent vectors, core/configuration, WARP parity, real capture and app
  automation. Reference generation is developer-only, not a build/runtime dependency.

Most recent fixes: cached static-frame updates; pacing that counts Present time;
idle FPS measurement; strict array shapes; occupied-hotkey rollback and exe-only launch.

## Current task and blockers

The reviewable executable/package milestone is complete. Next is the hardware
acceptance matrix. The full original definition of done is **not yet satisfied**:
physical multiple monitors, mixed DPI/refresh/adapters, Windows 10/11 hardware coverage,
resolution/disconnection/device loss/crash recovery, video/borderless games, ReShade
compilation/game checks and 1080p/1440p/4K benchmarks remain open.

No hardware performance measurements exist. 60 FPS is a target. Exclusive fullscreen
is unsupported in overlay mode. HDR/ICC/wide-gamut and protected/secure surfaces are
outside this build. Rare/acquired presets remain explicit approximations.

## Decisions and lessons to preserve

Independent WGC/D3D11 implementation after ShaderGlass evaluation; Win32 controls;
SDR sRGB; recommended Machado protan/deutan and Brettel tritan; no game injection,
anti-cheat bypass or kernel changes. Do not import GPL code without changing licensing.

Pin reference generation to DaltonLens `3cba5e6a7c8f0e8199c8f83f1afb58eb6dab7a3d`:
older PyPI versions use a different color-space adaptation. Its Machado fractional
interpolation lacks a factor of ten, so use exact table points as reference fixtures
and test interpolation separately. Never alter correct code to match that defect.

Windows documents F12 as reserved; it registered on this runner but may fail elsewhere.
Preserve conflict reporting, configurable alternatives and fixed Ctrl+Alt+Home. Filtering
must start off. Emergency polling cannot guarantee recovery from an OS/driver hang.

## Resume commands and persistent workspace

Windows: `cmake -S . -B build -A x64`, `cmake --build build --config Release --parallel`,
`ctest --test-dir build -C Release --verbose`.
Portable core: omit `-A x64` and `--config Release`.

Read AGENTS.md, dashboard, roadmap, decisions, tests and git status; reconcile evidence.
Notion HQ: https://www.notion.so/4198721d702e8132a66ad28c995aa2d8
Notion handoff: https://www.notion.so/4198721d702e817ea851ddf70804d31a

Next: run the manual matrix with actual hardware details, fix observed failures, record
measurements and then decide whether production-release gates are met.
