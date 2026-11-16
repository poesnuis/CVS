# Known limitations and open validation

- Windows 10 2004+ / Windows 11 x64 and D3D11 feature level 11.0 are the intended
  platform. Tests ran on the `windows-2022` GitHub runner with one 1024×768 display;
  this is not a Windows 10/11 physical hardware compatibility matrix.
- SDR sRGB only. HDR outputs are refused. ICC/wide-gamut calibration is not implemented.
- Experimental rare/acquired conditions and partial Brettel severity are appearance
  approximations. Spatial acuity, photophobia, rods/adaptation and individual responses
  are not reproduced.
- Overlay cannot cover exclusive fullscreen bypassing composition. Borderless game
  compatibility is unverified. ReShade .fx is supplied but has not been validated in
  ReShade or games; its preset subset is smaller than the desktop preset manager.
- The original build lost filtering over focused FNaF 1 on Windows 11 build 26100,
  while the rest of the desktop stayed filtered (client report). Overlay order recovery
  now targets foreground/topmost changes; an actual FNaF retest is still required.
- DRM/protected content, secure desktop and lock screen are unsupported. The capture
  indicator, hardware cursor and some higher-z-order system surfaces remain unfiltered.
- Multiple displays and resolution/DPI/disconnect recovery are implemented but have
  not been exercised on physical mixed-monitor hardware. Device loss retries are
  bounded; actual driver-reset and crash/fault injection remain unverified.
- Windows documents F12 as debugger-reserved. It registered on the test runner, but
  may fail elsewhere. The app reports failures and supports another chord such as
  Ctrl+Alt+PageDown; it does not intercept reserved keys with hooks.
- The emergency shortcut and normal shutdown were tested. An OS/graphics-driver hang
  can defeat user-mode recovery; terminating the process removes its windows when the
  OS remains responsive. No claim of guaranteed recovery from a system hang.
- 60 FPS and 1080p/1440p/4K are targets, not measured benchmarks. Diagnostics report
  submitted frame rate, GPU draw time and capture-to-submit frame age, not end-to-end
  latency. Static displays may produce fewer fresh frames.
- The executable is unsigned. This is a development package with outstanding
  hardware/game acceptance gates. Original implementation PR #1 was merged by the owner.
