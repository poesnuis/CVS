# Roadmap mirror

Authoritative evidence is code and test output. Notion is the persistent tracker:
https://www.notion.so/4198721d702e8182b52ae046729df141

| Task | Status | Verification / next action |
|---|---|---|
| R01 Capture/licensing research | Done | Sources, comparison and decisions recorded |
| R02 Monitor capture/exclusion | Testing | Real single-display capture/exclusion/output/input passes; long app-switching/video/multi-monitor checks remain |
| R03 Scientific transforms | Done | Published values, 96 independent fixtures, interpolation/invariants/gamut and WARP parity |
| R04 Rare/experimental/artistic controls | Done | All required modes implemented, labeled, validated by parameter/core/GPU coverage; appearance proxies remain experimental |
| R05 GUI/preview/monitor controls | Testing | Screenshot reviewed and primary UI handlers exercised; multi-DPI/monitor selection hardware matrix remains |
| R06 JSON persistence/import/export | Done | Validated schema, atomic replacement/round trip, malformed input rejection, GUI save/load; file picker remains manual |
| R07 Tray/configurable hotkeys | Testing | Registration, real toggle/next, conflict rollback and emergency checks; full menu and every customized chord remain manual |
| R08 Optional ReShade integration | Testing | Source shader and guide delivered; ReShade FX compilation and game validation pending |
| R09 Recovery/safety | Testing | Normal and emergency lifecycle passes; display/device-loss/crash scenarios remain |
| R10 Diagnostics | Testing | Runtime FPS, query timing, frame age and process memory implemented; profiling accuracy on real hardware pending |
| R11 Hardware compatibility/benchmarks | Blocked | Needs Windows 10/11 hardware, multiple displays and permitted games |
| R12 Standalone package | Done | Real MSVC x64 executable, embedded shader, source archive, licenses and examples |
| R13 Automated Windows tests | Done | Latest actual run recorded in TESTING.md |
| R14 Evidence/handoff | Done | Local docs and Notion checkpoint reconciled; latest artifact provenance in HANDOFF.md |

Priority order: validate recovery and multiple monitors; test Windows 10/11 desktop/video
and borderless games; measure 1080p/1440p/4K; compile/test optional ReShade; then address
findings before cosmetic work. Each test should record machine/OS/GPU/driver/display
configuration, procedure and observed result.
