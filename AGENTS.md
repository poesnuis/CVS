# CVS development sessions

Read `docs/HANDOFF.md`, `docs/TESTING.md`, `docs/ARCHITECTURE.md`, and git status first.
Notion project home: https://www.notion.so/4198721d702e8132a66ad28c995aa2d8
Handoff: https://www.notion.so/4198721d702e817ea851ddf70804d31a
Roadmap data source: collection://4198721d-702e-810b-a842-000baa0e2c30
Bug data source: collection://4198721d-702e-811c-a38c-000b556742f5

Read the dashboard, roadmap, decisions and handoff through the authenticated Notion
connector. Reconcile with actual code and tests. Do not assume a Notion status proves
behavior. Update meaningful milestones, failures, tests and next actions in Notion
and repository documentation. If Notion is unavailable, continue with Markdown and
record updates awaiting synchronization. Never store credentials here or in Notion.

Windows build: `cmake -S . -B build -A x64`,
`cmake --build build --config Release --parallel`,
`ctest --test-dir build -C Release --output-on-failure`.
Portable core: same commands without `-A x64` / `--config Release`.
Workflow: `.github/workflows/windows.yml`; download real artifacts and inspect them
before claiming an exe exists. Use the current commit's results, not a stale run.

Preserve capture exclusion, disabled-at-start behavior, emergency disable,
click-through overlays, linear sRGB and honest scientific labels. No game injection,
anti-cheat bypass, stealth or kernel modifications. Do not publicly release builds
without authorization. A workflow artifact is the reviewable development package.
Hardware tests and benchmarks must record actual environment and measurements.
ReShade and borderless/exclusive compatibility remain unverified until tested.

Use the existing source components, independent reference vectors and GPU tests.
Keep coefficients traceable and third-party licenses included. Update the handoff
with branch/commit, build result, active task and blockers before ending a session.
