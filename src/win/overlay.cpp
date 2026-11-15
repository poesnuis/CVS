#include "overlay.h"
#include "capture.h"

namespace cvs {
namespace {
void raise_settings(HWND settings) {
    if (!settings || !IsWindowVisible(settings) || IsIconic(settings))
        return;
    constexpr UINT flags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER;
    SetWindowPos(settings, HWND_TOPMOST, 0, 0, 0, 0, flags);
    // Preserve a file picker/message box above its owner without stealing focus.
    HWND popup = GetLastActivePopup(settings);
    if (popup != settings && IsWindowVisible(popup))
        SetWindowPos(popup, HWND_TOPMOST, 0, 0, 0, 0, flags);
}
} // namespace
bool maintain_overlay_order(HWND overlay, HWND settings) {
    if (!IsWindowVisible(overlay))
        return false; // capture failure, startup and emergency hiding must stay hidden
    HWND foreground = GetForegroundWindow();
    if (!foreground || foreground == overlay || !IsWindowVisible(foreground) ||
        IsIconic(foreground) || GetAncestor(foreground, GA_ROOTOWNER) == settings)
        return false;
    RECT filterRect{}, foregroundRect{}, intersection{};
    if (!GetWindowRect(overlay, &filterRect) || !GetWindowRect(foreground, &foregroundRect) ||
        !IntersectRect(&intersection, &filterRect, &foregroundRect))
        return false;
    // TOPMOST is a band, not a permanent position. A game's later SetWindowPos or
    // activation can put it above us. Only repair an actual ordering violation.
    for (HWND above = GetWindow(overlay, GW_HWNDPREV); above;
         above = GetWindow(above, GW_HWNDPREV)) {
        if (above != foreground)
            continue;
        if (!SetWindowPos(overlay, HWND_TOPMOST, 0, 0, 0, 0,
                          SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER))
            return false;
        raise_settings(settings);
        return true;
    }
    return false;
}
LRESULT CALLBACK overlay_procedure(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    if (m == WM_NCCREATE) {
        auto create = reinterpret_cast<const CREATESTRUCTW *>(lp);
        SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    if (m == WM_NCHITTEST)
        return HTTRANSPARENT;
    if (m == WM_MOUSEACTIVATE)
        return MA_NOACTIVATE;
    if (m == overlayHide) {
        ShowWindow(w, SW_HIDE);
        return 0;
    }
    if (m == overlayShow) {
        SetWindowPos(w, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
        raise_settings(reinterpret_cast<HWND>(GetWindowLongPtrW(w, GWLP_USERDATA)));
        return 0;
    }
    if (m == WM_ERASEBKGND)
        return 1;
    return DefWindowProcW(w, m, wp, lp);
}
} // namespace cvs
