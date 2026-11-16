#pragma once
#include <windows.h>

namespace cvs {
// Overlay HWNDs are independent top-level windows. An owner would hide them when
// settings is minimized, and force them above the settings window itself.
LRESULT CALLBACK overlay_procedure(HWND, UINT, WPARAM, LPARAM);
// Recover from a foreground game raising itself above the filter. Never activate
// a CVS window, show a hidden overlay, or interfere with CVS modal dialogs.
bool maintain_overlay_order(HWND overlay, HWND settings);
} // namespace cvs
