# Development journal

## Initial implementation — 2026-11-14, America/New_York

Found `poesnuis/CVS` with README-only initial commit
`c4fb478d8cf467ba895ee9023b77b0f72fc59186`. Created the authorized Notion headquarters,
roadmap, bug tracker, research library, decision log, testing and handoff pages.

Evaluated ShaderGlass (GPL-3.0), Windows Graphics Capture, Desktop Duplication,
Machado/Brettel science and ReShade. Chose independently implemented WGC + D3D11,
native Win32 controls, SDR sRGB and explicit approximation labels.

Implemented the core, capture workers, renderer/HLSL, UI, tray/hotkeys, JSON presets,
diagnostics, recovery, optional shader and packaging. First portable compile exposed
ambiguous matrix/vector aggregate overload resolution; an explicit Vec3 fixed it.
The initial GCC Release suite passed 17,412 assertions.

## Windows build and scientific reference work

First MSVC build at `bd7383b` failed on missing `<string>` includes and int/LONG
template deduction. `02bd348` fixed both; MSVC, CPU tests, 69,120 WARP comparisons,
application launch and artifact creation then passed.

Independent fixtures exposed a reference-version difference: DaltonLens PyPI 0.1.5
uses a different RGB/XYZ adaptation from its current Judd-Vos model. Pinned source
`3cba5e6a7c8f0e8199c8f83f1afb58eb6dab7a3d` matches CVS. Also found a factor-of-ten
omission in that reference's fractional Machado interpolation. Use its exact tabulated
Machado steps and Brettel outputs as external fixtures; test interpolation separately.
Do not copy the reference defect. 96 independent vectors pass (288 assertions).

Corrected static-desktop parameter changes to redraw a cached GPU frame, counted
Present time toward frame pacing, and reset measured FPS during idle intervals.
These were code findings, not claimed hardware benchmark improvements.

## Real desktop and application validation

The Windows runner supported actual capture. Integration tests display a changing
RGB window beneath the excluded overlay, read test-only GPU pixels, compare output
with the reference, and inject a click through the overlay. At `d3415cd`, 121 samples
passed at 1024×768, maximum UNORM error 0.00183153, with one delivered click.
The WARP comparison maximum channel error was 5.77653e-07.

Application automation exercised real registered F8/F9 key events, severity controls,
preset-manager save/load, tray registration, re-enabling and Ctrl+Alt+Home. The GUI
screenshot was inspected and showed readable native controls and paired color preview.
The runner registered all requested default hotkeys, including F12; Windows documents
F12 as reserved, so other systems still need conflict reporting/customization.

One CI run failed only at test-log upload because an artifact spanned C: and D:.
Copied smoke evidence into `build/` to resolve the cross-drive artifact-root error.
Binary packaging and tests had already succeeded in that run; no application fix was
claimed for that upload issue.

## Continued checkpoint — 2026-11-15

Notion writes briefly failed because automatic approval review hit an account usage
limit. After the user continued, access was checked again, stale dashboard/handoff
state was reconciled, and writes resumed. No alternate API bypass was used.

Final hardening rejects incorrect matrix/cone-array lengths, tests occupied global
shortcut rollback, and launches a copy of the exe from an isolated directory with a
space in its name. Tests and the final artifact are recorded in TESTING.md/HANDOFF.md.

Unresolved: physical multiple monitors, Windows 10/11 hardware coverage, display and
driver fault injection, video/borderless games, ReShade FX/game validation and hardware
benchmarks. Continue with these acceptance gates before calling the full specification
complete.
