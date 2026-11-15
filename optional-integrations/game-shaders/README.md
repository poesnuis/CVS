# Optional game shader integration

`ColorVisionSimulator.fx` is an original ReShade FX shader, with the same Machado
severity tables and Brettel coefficients as the desktop core. It supplies protan,
deutan, tritan and luminance-grayscale approximation modes, severity and separate
artistic saturation/exposure/contrast. The 15-preset desktop manager and rare-condition
controls are not integrated with ReShade. Partial Brettel severity remains a blend.

This optional file has **not been compiled by ReShade or tested in a game** in this
session. CPU/core HLSL tests do not verify ReShade FX compilation. Treat it as an
unverified integration until tested; report the ReShade version, API and game.

1. Check that the game permits ReShade. Do not use it where prohibited by game or
   anti-cheat policy. CVS provides no installation/injection or bypass functionality.
2. Obtain and install standard ReShade from https://reshade.me/ using its official
   setup instructions for a supported game. CVS does not redistribute its binaries.
3. Copy this `.fx` file to that installation's `reshade-shaders/Shaders` directory.
4. Reload effects in ReShade, enable `ColorVisionSimulator`, and choose settings.
5. Disable CVS desktop filtering on that game's monitor to avoid double simulation.
6. Use SDR output and verify that the sampled backbuffer is sRGB encoded; HDR or
   linear/custom game pipelines require a separately validated conversion.

ReShade supports multiple graphics APIs but actual exclusive/fullscreen behavior
depends on the game, API, presentation path and allowed integration. It is not an
assertion that CVS desktop overlays support exclusive fullscreen. Do not modify kernel
graphics components or bypass anti-cheat. No online-game compatibility is claimed.

Preserve the source/license notices if redistributing this shader. See LICENSES and
docs/SCIENCE.md in the CVS distribution for coefficient provenance and model limits.
