# Architecture and decisions

ADR-001 (2026-11-14): independently implement WGC + D3D11, using native Win32 controls.
ShaderGlass was evaluated (GPL-3.0, WGC, D3D11, Windows 10 2004 minimum for glass).
A fork would import its large shader/backend stack. Desktop Duplication is a valid
alternative but needs its own access-loss, rotation and recursion validation. No
untested capture fallback is offered.

The main thread owns controls and excluded, layered, transparent, non-activating,
topmost overlay HWNDs. One `Capture` worker per selected monitor owns an adapter-local
D3D11 device, free-threaded WGC frame pool, two-buffer flip swap chain, sampling texture
and renderer. Frames are drained to the latest available; one GPU copy allows shader
sampling regardless of capture texture bind flags. One full-screen triangle performs
manual sRGB decode, scientific transform, artistic transform, clipping and encode.

Immediate-context multithread protection is enabled for WGC interoperability.
Settings/stats are exchanged under mutexes. Timestamp queries use a ring and nonblocking
GetData; no frame processing readback. WARP tests intentionally read pixels back in tests
only. Display refresh and user FPS limit constrain updates; hardware throughput is unknown.

Capture exclusion is checked before showing any overlay; failure aborts activation.
OS build 19041+ is checked because older systems interpret exclusion as a black window.
The main UI is also excluded, and placed above the overlay so its paired preview stays
visible. Some other topmost system surfaces may appear above overlays. Cursor capture
is disabled; the actual cursor remains interactive and unfiltered.

Errors hide overlays and retry up to three times. Display changes hide then rebuild
capture. Shutdown hides windows before worker joins; process death destroys windows.
Ctrl+Alt+Home uses RegisterHotKey and separate polling; severe driver/OS/UI hangs still
require process termination. Filtering never automatically resumes on launch or sleep.

ADR-002: SDR sRGB only. HDR output detection refuses activation; gamut/ICC/advanced-color
support needs a calibrated pipeline and hardware validation. White-neutral Brettel
adaptation is chosen for sRGB display stability, with source-specific anchors retained.

ADR-003: rare/acquired conditions are explicitly experimental approximations. Artistic
adjustments are separate. The common transform parameter structure is shared by CPU
preview and shader to make parity testable; CPU references additionally need independent
published/third-party vectors, not just CPU/GPU agreement.

Reconsider WGC if measured driver compatibility or latency fails acceptance criteria.
Preserve this decision history when adding another capture backend. Do not imply an
exclusive fullscreen path exists in the overlay renderer.
