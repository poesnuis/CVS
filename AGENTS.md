# CVS development sessions

Read `docs/TESTING.md`, `docs/ARCHITECTURE.md`, `docs/KNOWN_ISSUES.md`, and git status
first. Compare the current branch with the remote before editing. Record technical
behavior and verified tests in the repository; keep private project management and
workspace links out of the source tree, pull request descriptions and packages.
Never store credentials in the repository.

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
Keep coefficients traceable and third-party licenses included. Report the current
branch/commit, build result and remaining validation at the end of each change.
Overlay windows must be unowned, excluded, click-through and non-activating. Verify
foreground topmost-window recovery, input, minimization and emergency hiding on Windows.
