# Development Handoff — Current State

Session date: 2026-11-14 (America/New_York).
Repository: https://github.com/poesnuis/CVS
Branch: `codex/color-vision-simulator` (base `c4fb478d8cf467ba895ee9023b77b0f72fc59186`).

Implementation: shared scientific transforms, JSON configuration, WGC/D3D11 per-monitor
capture, native GUI, preview, tray, hotkeys and emergency controls. Windows CI includes
MSVC Release build, core tests, WARP shader parity and non-capture GUI smoke test.

Latest verified milestone: portable Release core tests pass on GCC 14.2 Linux.
Active task: build/test Windows application, fix failures, inspect genuine artifact.
Latest Windows build: pending first workflow run.

Key decisions: WGC; independent code after ShaderGlass evaluation; Win32 controls;
SDR sRGB only; rare/acquired models experimental; no injection or anti-cheat bypass.
Windows reserves F12; requested default is attempted and conflicts are reported.

Blocker: no local Windows graphics desktop. Real capture exclusion, interaction,
multi-monitor, borderless/fullscreen game behavior, driver recovery and benchmarks
remain unverified. Do not mark those requirements complete from compilation alone.

Build/test commands: README and AGENTS.md. Sources: src/core, src/win, shaders.
Next: complete Windows CI and artifact verification; then execute docs/TESTING.md on
Windows hardware, record actual measurements and correct any issues before release.

Notion HQ: https://www.notion.so/4198721d702e8132a66ad28c995aa2d8
Notion handoff: https://www.notion.so/4198721d702e817ea851ddf70804d31a
