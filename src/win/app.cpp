#include "app.h"
#include "overlay.h"
#include <algorithm>
#include <cmath>
#include <commctrl.h>
#include <commdlg.h>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <psapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <sstream>
#include <stdexcept>
#include <windowsx.h>

namespace cvs {
enum Id {
    Enable = 100,
    Pause,
    ToTray,
    Status,
    Tab,
    Search,
    PresetList,
    Description,
    Severity,
    SeverityText,
    ModelSelect,
    ModeSelect,
    MonitorList,
    AllMonitors,
    RefreshMonitors,
    Preview,
    ModelNote,
    ConeL = 150,
    ConeM,
    ConeS,
    ConeSurvivor,
    Saturation,
    Brightness,
    Contrast,
    AdvancedValues,
    ResetArtistic,
    Matrix0 = 180,
    MatrixApply = 189,
    SavedList = 200,
    PresetName,
    SavePreset,
    LoadPreset,
    DeletePreset,
    ImportPreset,
    ExportPreset,
    Hotkey0 = 220,
    ApplyHotkeys = 225,
    HotkeyStatus,
    FpsLimit,
    MinimizeTray,
    Compatibility,
    Diagnostics,
    OpenLogs,
    TrayToggle = 300,
    TrayPause,
    TrayNext,
    TrayPrevious,
    TrayOpen,
    TrayExit,
    TrayPreset = 400
};
constexpr UINT trayMessage = WM_APP + 60;
std::wstring wide(std::string_view s) {
    if (s.empty())
        return {};
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()),
                                nullptr, 0);
    if (!n)
        return L"Invalid UTF-8";
    std::wstring r(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), r.data(), n);
    return r;
}
std::string utf8(std::wstring_view s) {
    if (s.empty())
        return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0,
                                nullptr, nullptr);
    std::string r(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), r.data(), n, nullptr,
                        nullptr);
    return r;
}
static std::wstring text(HWND w) {
    int n = GetWindowTextLengthW(w);
    std::wstring s(n + 1, L'\0');
    GetWindowTextW(w, s.data(), n + 1);
    s.resize(n);
    return s;
}
static void set(HWND w, const std::wstring &s) {
    SetWindowTextW(w, s.c_str());
}
static bool checked(HWND w) {
    return SendMessageW(w, BM_GETCHECK, 0, 0) == BST_CHECKED;
}
static int selection(HWND w) {
    return static_cast<int>(SendMessageW(w, CB_GETCURSEL, 0, 0));
}
static int list_selection(HWND w) {
    return static_cast<int>(SendMessageW(w, LB_GETCURSEL, 0, 0));
}
static std::wstring fixed(double d, int decimals = 1) {
    std::wostringstream s;
    s << std::fixed << std::setprecision(decimals) << d;
    return s.str();
}
static void send_test_hotkey(WORD key) {
    INPUT input[6]{};
    WORD keys[]{VK_CONTROL, VK_MENU, key, key, VK_MENU, VK_CONTROL};
    for (int i = 0; i < 6; ++i) {
        input[i].type = INPUT_KEYBOARD;
        input[i].ki.wVk = keys[i];
        if (i >= 3)
            input[i].ki.dwFlags = KEYEVENTF_KEYUP;
    }
    if (SendInput(6, input, sizeof(INPUT)) != 6)
        throw std::runtime_error("Could not send smoke-test hotkey");
}
static bool test_window_above(HWND upper, HWND lower) {
    // Observe actual desktop ordering; do not call the production recovery helper.
    for (HWND w = GetTopWindow(nullptr); w; w = GetWindow(w, GW_HWNDNEXT)) {
        if (w == upper)
            return true;
        if (w == lower)
            return false;
    }
    return false;
}
App::App(HINSTANCE instance, bool smoke, bool safe) : instance_(instance), smoke_(smoke) {
    PWSTR local{};
    check_hr(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &local),
             "Find settings folder");
    auto folder = std::filesystem::path(local) / L"ColorVisionSimulator";
    CoTaskMemFree(local);
    if (smoke_)
        folder = std::filesystem::temp_directory_path() / L"CVS-smoke";
    configPath_ = folder / L"settings.json";
    logPath_ = folder / L"diagnostics.log";
    if (!safe && !smoke_ && std::filesystem::exists(configPath_)) {
        try {
            config_ = load_config(configPath_);
        } catch (const std::exception &e) {
            notice_ = L"Invalid settings preserved. Defaults loaded; corrected settings will be "
                      L"saved separately. " +
                      wide(e.what());
            configPath_ = folder / L"settings.recovered.json";
            log(e.what());
        }
    }
    WNDCLASSEXW cls{sizeof(cls)};
    cls.hInstance = instance_;
    cls.lpszClassName = appClass;
    cls.lpfnWndProc = procedure;
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    if (!RegisterClassExW(&cls))
        throw std::runtime_error("Cannot register main window");
    cls.lpszClassName = L"CVS.Overlay.1";
    cls.lpfnWndProc = overlay_procedure;
    cls.hbrBackground = nullptr;
    RegisterClassExW(&cls);
    dpi_ = static_cast<int>(GetDpiForSystem());
    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int workWidth = static_cast<int>(work.right - work.left),
        workHeight = static_cast<int>(work.bottom - work.top);
    int w = std::min(MulDiv(840, dpi_, 96), workWidth),
        h = std::min(MulDiv(780, dpi_, 96), workHeight);
    window_ = CreateWindowExW(
        0, appClass, L"Color Vision Simulator", WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        work.left + std::max(0, (workWidth - w) / 2), work.top + std::max(0, (workHeight - h) / 2),
        w, h, nullptr, nullptr, instance_, this);
    if (!window_)
        throw std::runtime_error("Cannot create main window");
    dpi_ = static_cast<int>(GetDpiForWindow(window_));
    // Keep the preview out of the monitor capture. The UI is shown above overlays.
    if (!SetWindowDisplayAffinity(window_, WDA_EXCLUDEFROMCAPTURE))
        log("Main window capture exclusion failed; preview may be filtered by desktop overlay");
    create_controls();
    reload_monitors();
    refresh_controls();
    register_hotkeys();
    tray(true);
    taskbarCreated_ = RegisterWindowMessageW(L"TaskbarCreated");
    SetTimer(window_, 1, 100, nullptr);
    if (smoke_)
        SetTimer(window_, 2, 1800, nullptr);
    watchdog_ = std::jthread([this](std::stop_token stop) {
        while (!stop.stop_requested()) {
            bool chord = (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
                         (GetAsyncKeyState(VK_MENU) & 0x8000) &&
                         (GetAsyncKeyState(VK_HOME) & 0x8000);
            if (chord) {
                emergency_ = true;
                std::lock_guard lock(safetyMutex_);
                for (auto h : safetyWindows_)
                    ShowWindowAsync(h, SW_HIDE);
                PostMessageW(window_, emergencyMessage, 0, 0);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
    });
}
App::~App() {
    watchdog_.request_stop();
    if (watchdog_.joinable())
        watchdog_.join();
    stop();
    if (smokeGame_)
        DestroyWindow(smokeGame_);
    if (window_) {
        for (int i = 0; i < 5; ++i)
            UnregisterHotKey(window_, i + 1);
        UnregisterHotKey(window_, 10);
        tray(false);
    }
    if (font_)
        DeleteObject(font_);
}
void App::log(const std::string &s) {
    try {
        std::filesystem::create_directories(logPath_.parent_path());
        if (std::filesystem::exists(logPath_) &&
            std::filesystem::file_size(logPath_) > 1024 * 1024) {
            auto prev = logPath_;
            prev += L".previous";
            std::error_code ec;
            std::filesystem::remove(prev, ec);
            std::filesystem::rename(logPath_, prev, ec);
        }
        std::ofstream out(logPath_, std::ios::app);
        SYSTEMTIME t{};
        GetLocalTime(&t);
        out << t.wYear << '-' << t.wMonth << '-' << t.wDay << ' ' << t.wHour << ':' << t.wMinute
            << ':' << t.wSecond << " " << s << '\n';
    } catch (...) {
    }
}
void App::report(const std::exception &e) {
    log(e.what());
    notice_ = wide(e.what());
    set(control(Status), notice_);
    if (!smoke_)
        MessageBoxW(window_, notice_.c_str(), L"Color Vision Simulator", MB_OK | MB_ICONWARNING);
}
HWND App::control(int id) const {
    return GetDlgItem(window_, id);
}
HWND App::add(int id, const wchar_t *cls, const wchar_t *label, DWORD style, int tab, int x, int y,
              int w, int h) {
    HWND child = CreateWindowExW(
        (wcscmp(cls, L"EDIT") == 0 || wcscmp(cls, L"LISTBOX") == 0) ? WS_EX_CLIENTEDGE : 0, cls,
        label, WS_CHILD | WS_VISIBLE | style, 0, 0, 1, 1, window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
    if (!child)
        throw std::runtime_error("Cannot create GUI control");
    controls_.push_back({child, tab, x, y, w, h});
    return child;
}
void App::create_controls() {
    auto label = [&](const wchar_t *s, int tab, int x, int y, int w, int h = 22) {
        add(0, L"STATIC", s, 0, tab, x, y, w, h);
    };
    auto button = [&](int id, const wchar_t *s, int tab, int x, int y, int w) {
        add(id, L"BUTTON", s, WS_TABSTOP | BS_PUSHBUTTON, tab, x, y, w, 28);
    };
    auto combo = [&](int id, std::initializer_list<const wchar_t *> values, int tab, int x, int y,
                     int w) {
        auto h = add(id, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, tab, x, y, w,
                     180);
        for (auto v : values)
            SendMessageW(h, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v));
    };
    auto slider = [&](int id, int tab, int x, int y, int w, int lo, int hi) {
        auto h =
            add(id, TRACKBAR_CLASSW, L"", WS_TABSTOP | TBS_HORZ | TBS_AUTOTICKS, tab, x, y, w, 34);
        SendMessageW(h, TBM_SETRANGE, TRUE, MAKELPARAM(lo, hi));
        SendMessageW(h, TBM_SETTICFREQ, 10, 0);
    };
    add(Enable, L"BUTTON", L"Enable desktop filtering", WS_TABSTOP | BS_AUTOCHECKBOX, -1, 20, 16,
        230, 28);
    button(Pause, L"Pause", -1, 266, 16, 90);
    button(ToTray, L"Hide to tray", -1, 366, 16, 110);
    label(L"Emergency disable: Ctrl + Alt + Home", -1, 492, 21, 310);
    auto tabs = add(Tab, WC_TABCONTROLW, L"", WS_TABSTOP, -1, 12, 56, 792, 32);
    const wchar_t *titles[]{L"Simulation", L"Advanced", L"Presets", L"Settings & compatibility"};
    for (int i = 0; i < 4; ++i) {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t *>(titles[i]);
        TabCtrl_InsertItem(tabs, i, &item);
    }
    label(L"Search presets", 0, 24, 108, 240);
    add(Search, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, 0, 24, 132, 270, 25);
    add(PresetList, L"LISTBOX", L"", WS_TABSTOP | LBS_NOTIFY | WS_VSCROLL, 0, 24, 164, 270, 205);
    label(L"Severity", 0, 318, 110, 180);
    add(SeverityText, L"STATIC", L"100%", 0, 0, 680, 110, 90, 22);
    slider(Severity, 0, 312, 138, 462, 0, 100);
    label(L"Scientific model", 0, 318, 190, 180);
    combo(ModelSelect,
          {L"Recommended for preset", L"Machado 2009", L"Brettel 1997 (partial severity = blend)"},
          0, 318, 215, 456);
    add(Description, L"STATIC", L"", 0, 0, 318, 252, 456, 53);
    add(ModelNote, L"STATIC", L"", 0, 0, 318, 310, 456, 50);
    label(L"Rendering mode", 0, 24, 386, 140);
    combo(ModeSelect, {L"Desktop overlay", L"Game shader (external ReShade)"}, 0, 170, 382, 304);
    add(AllMonitors, L"BUTTON", L"All displays", WS_TABSTOP | BS_AUTOCHECKBOX, 0, 500, 382, 130,
        28);
    button(RefreshMonitors, L"Refresh displays", 0, 640, 382, 134);
    label(L"Displays to filter (click to select multiple)", 0, 24, 420, 500);
    add(MonitorList, L"LISTBOX", L"", WS_TABSTOP | LBS_MULTIPLESEL | LBS_NOTIFY | WS_VSCROLL, 0, 24,
        448, 750, 66);
    add(Preview, L"STATIC", L"", SS_OWNERDRAW, 0, 24, 534, 750, 116);
    label(L"SDR sRGB preview. Hardware cursor, protected surfaces and system capture border are "
          L"not simulated.",
          0, 24, 658, 750, 40);
    label(L"Experimental cone response controls (used only by the relevant preset)", 1, 24, 110,
          750);
    label(L"L cone", 1, 24, 150, 80);
    slider(ConeL, 1, 110, 143, 240, 0, 200);
    label(L"M cone", 1, 390, 150, 80);
    slider(ConeM, 1, 476, 143, 290, 0, 200);
    label(L"S cone", 1, 24, 195, 80);
    slider(ConeS, 1, 110, 188, 240, 0, 200);
    label(L"Surviving cone", 1, 390, 195, 130);
    combo(ConeSurvivor, {L"L", L"M", L"S"}, 1, 530, 190, 236);
    label(L"Custom 3 x 3 matrix in linear sRGB (select Custom RGB matrix preset)", 1, 24, 242, 740);
    for (int i = 0; i < 9; ++i)
        add(Matrix0 + i, L"EDIT", i % 4 == 0 ? L"1" : L"0", WS_TABSTOP | ES_AUTOHSCROLL, 1,
            24 + (i % 3) * 130, 275 + (i / 3) * 35, 116, 26);
    button(MatrixApply, L"Apply matrix", 1, 430, 310, 160);
    label(L"Artistic adjustments — separate from scientific simulation", 1, 24, 405, 750);
    label(L"Saturation", 1, 24, 447, 120);
    slider(Saturation, 1, 150, 440, 610, 0, 200);
    label(L"Brightness (EV)", 1, 24, 490, 120);
    slider(Brightness, 1, 150, 483, 610, -200, 200);
    label(L"Contrast", 1, 24, 533, 120);
    slider(Contrast, 1, 150, 526, 610, 0, 200);
    add(AdvancedValues, L"STATIC", L"", 0, 1, 24, 580, 740, 44);
    button(ResetArtistic, L"Reset artistic adjustments", 1, 24, 635, 235);
    label(L"Saved presets", 2, 24, 110, 500);
    add(SavedList, L"LISTBOX", L"", WS_TABSTOP | LBS_NOTIFY | WS_VSCROLL, 2, 24, 140, 480, 320);
    button(LoadPreset, L"Load selected", 2, 530, 140, 230);
    button(DeletePreset, L"Delete selected", 2, 530, 180, 230);
    label(L"Name for current settings", 2, 24, 480, 500);
    add(PresetName, L"EDIT", L"My preset", WS_TABSTOP | ES_AUTOHSCROLL, 2, 24, 510, 480, 28);
    SendMessageW(control(PresetName), EM_SETLIMITTEXT, 80, 0);
    button(SavePreset, L"Save / replace by name", 2, 530, 510, 230);
    button(ImportPreset, L"Import JSON...", 2, 24, 570, 180);
    button(ExportPreset, L"Export JSON...", 2, 220, 570, 180);
    label(L"Import merges named presets and loads the imported current settings. Export includes "
          L"current and saved presets. Invalid files are rejected before changing settings.",
          2, 24, 626, 735, 60);
    const wchar_t *actions[]{L"Toggle simulation", L"Next preset", L"Previous preset",
                             L"Increase severity", L"Decrease severity"};
    label(L"Global shortcuts (click a field and press the desired combination)", 3, 24, 110, 750);
    for (int i = 0; i < 5; ++i) {
        label(actions[i], 3, 24, 150 + i * 35, 200);
        add(Hotkey0 + i, HOTKEY_CLASSW, L"", WS_TABSTOP, 3, 230, 145 + i * 35, 230, 26);
    }
    button(ApplyHotkeys, L"Apply shortcuts", 3, 494, 145, 250);
    add(HotkeyStatus, L"STATIC", L"", 0, 3, 494, 185, 270, 140);
    label(L"Maximum FPS", 3, 24, 343, 160);
    add(FpsLimit, L"EDIT", L"60", WS_TABSTOP | ES_NUMBER, 3, 190, 338, 90, 28);
    add(MinimizeTray, L"BUTTON", L"Close button hides to tray", WS_TABSTOP | BS_AUTOCHECKBOX, 3,
        324, 338, 320, 28);
    label(L"FPS change applies when this field loses focus (15–240). Capture/compositor may "
          L"deliver fewer frames.",
          3, 24, 377, 750, 40);
    add(Compatibility, L"STATIC",
        L"Windows 10 2004+ / Windows 11, x64. SDR only.\r\nOverlay: composited desktop, video and "
        L"compatible borderless applications.\r\nExclusive fullscreen: use the optional game "
        L"shader where ReShade is allowed.\r\nNo DRM/secure desktop capture, injection, anti-cheat "
        L"bypass or kernel changes.\r\nThe Windows capture indicator may remain visible. "
        L"Multi-monitor and game compatibility require hardware testing.\r\nF12 is reserved by "
        L"Windows; choose another key if registration fails.",
        0, 3, 24, 425, 750, 145);
    add(Diagnostics, L"EDIT", L"", ES_MULTILINE | ES_READONLY | WS_VSCROLL, 3, 24, 580, 750, 80);
    button(OpenLogs, L"Open diagnostics folder", 3, 24, 672, 235);
    add(Status, L"STATIC", L"Filtering is off. Select a preset and display, then enable.", 0, -1,
        20, 720, 774, 54);
    filter_presets();
    refresh_saved();
    layout();
}
void App::layout() {
    if (!window_)
        return;
    RECT client{};
    GetClientRect(window_, &client);
    SCROLLINFO si{sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS};
    si.nMin = 0;
    si.nMax = MulDiv(782, dpi_, 96);
    si.nPage = client.bottom;
    si.nPos = scroll_;
    SetScrollInfo(window_, SB_VERT, &si, TRUE);
    GetScrollInfo(window_, SB_VERT, &si);
    scroll_ = si.nPos;
    if (font_)
        DeleteObject(font_);
    font_ = CreateFontW(-MulDiv(14, dpi_, 96), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        DEFAULT_PITCH, L"Segoe UI");
    for (auto c : controls_) {
        MoveWindow(c.hwnd, MulDiv(c.x, dpi_, 96), MulDiv(c.y, dpi_, 96) - scroll_,
                   MulDiv(c.w, dpi_, 96), MulDiv(c.h, dpi_, 96), TRUE);
        SendMessageW(c.hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
        ShowWindow(c.hwnd, c.tab < 0 || c.tab == tab_ ? SW_SHOW : SW_HIDE);
    }
}
void App::filter_presets() {
    auto q = text(control(Search));
    std::transform(q.begin(), q.end(), q.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    visiblePresets_.clear();
    SendMessageW(control(PresetList), LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < 15; ++i) {
        auto name = wide(presets()[i].name), lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
        if (lower.find(q) != std::wstring::npos) {
            SendMessageW(control(PresetList), LB_ADDSTRING, 0,
                         reinterpret_cast<LPARAM>(name.c_str()));
            visiblePresets_.push_back(i);
        }
    }
    for (size_t i = 0; i < visiblePresets_.size(); ++i)
        if (visiblePresets_[i] == static_cast<int>(config_.settings.preset))
            SendMessageW(control(PresetList), LB_SETCURSEL, i, 0);
}
void App::refresh_saved() {
    SendMessageW(control(SavedList), LB_RESETCONTENT, 0, 0);
    for (const auto &p : config_.saved) {
        auto n = wide(p.name);
        SendMessageW(control(SavedList), LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(n.c_str()));
    }
}
void App::refresh_controls() {
    loading_ = true;
    auto &s = config_.settings;
    const auto &p = presets()[static_cast<int>(s.preset)];
    SendMessageW(control(Enable), BM_SETCHECK, enabled_ ? BST_CHECKED : BST_UNCHECKED, 0);
    set(control(Pause), paused_ ? L"Resume" : L"Pause");
    EnableWindow(control(Pause), enabled_);
    SendMessageW(control(Severity), TBM_SETPOS, TRUE,
                 static_cast<LPARAM>(std::lround(s.severity * 100)));
    set(control(SeverityText), fixed(s.severity * 100, 0) + L"%");
    SendMessageW(control(ModelSelect), CB_SETCURSEL, static_cast<int>(s.model), 0);
    set(control(Description), wide(p.description));
    std::wstring modelNote = p.experimental ? L"Experimental / artistic approximation."
                                            : L"Research-based sRGB appearance simulation.";
    if (s.model == Model::Brettel)
        modelNote += L" Partial severity is a blend, not an anomaly model.";
    if (s.model == Model::Machado &&
        (s.preset == Preset::Tritanopia || s.preset == Preset::Tritanomaly))
        modelNote += L" Machado tritan predictions have known limitations; Brettel is recommended.";
    set(control(ModelNote), modelNote);
    SendMessageW(control(AllMonitors), BM_SETCHECK,
                 config_.allMonitors ? BST_CHECKED : BST_UNCHECKED, 0);
    EnableWindow(control(MonitorList), !config_.allMonitors);
    for (int i = 0; i < 3; ++i) {
        SendMessageW(control(ConeL + i), TBM_SETPOS, TRUE, static_cast<LPARAM>(s.cones[i] * 100));
        EnableWindow(control(ConeL + i), s.preset == Preset::CustomLMS);
    }
    SendMessageW(control(ConeSurvivor), CB_SETCURSEL, s.survivingCone, 0);
    EnableWindow(control(ConeSurvivor), s.preset == Preset::ConeMonochromacy);
    for (int i = 0; i < 9; ++i) {
        set(control(Matrix0 + i), fixed(s.custom.v[i], 5));
        EnableWindow(control(Matrix0 + i), s.preset == Preset::CustomMatrix);
    }
    EnableWindow(control(MatrixApply), s.preset == Preset::CustomMatrix);
    SendMessageW(control(Saturation), TBM_SETPOS, TRUE, static_cast<LPARAM>(s.saturation * 100));
    SendMessageW(control(Brightness), TBM_SETPOS, TRUE, static_cast<LPARAM>(s.brightness * 100));
    SendMessageW(control(Contrast), TBM_SETPOS, TRUE, static_cast<LPARAM>(s.contrast * 100));
    set(control(AdvancedValues),
        L"L/M/S: " + fixed(s.cones[0]) + L" / " + fixed(s.cones[1]) + L" / " + fixed(s.cones[2]) +
            L"    Saturation: " + fixed(s.saturation) + L"    Brightness: " + fixed(s.brightness) +
            L" EV    Contrast: " + fixed(s.contrast));
    for (int i = 0; i < 5; ++i) {
        auto h = config_.hotkeys[i];
        BYTE mods = static_cast<BYTE>(((h.modifiers & MOD_ALT) ? HOTKEYF_ALT : 0) |
                                      ((h.modifiers & MOD_CONTROL) ? HOTKEYF_CONTROL : 0) |
                                      ((h.modifiers & MOD_SHIFT) ? HOTKEYF_SHIFT : 0));
        SendMessageW(control(Hotkey0 + i), HKM_SETHOTKEY, MAKEWORD(h.key, mods), 0);
    }
    SendMessageW(control(MinimizeTray), BM_SETCHECK,
                 config_.minimizeToTray ? BST_CHECKED : BST_UNCHECKED, 0);
    set(control(FpsLimit), std::to_wstring(config_.fpsLimit));
    set(control(HotkeyStatus), hotkeyStatus_);
    filter_presets();
    InvalidateRect(control(Preview), nullptr, FALSE);
    loading_ = false;
}
void App::reload_monitors() {
    bool resume = enabled_ && !paused_;
    stop();
    monitors_ = enumerate_monitors();
    SendMessageW(control(MonitorList), LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < monitors_.size(); ++i) {
        const auto &m = monitors_[i];
        SendMessageW(control(MonitorList), LB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(m.label.c_str()));
        bool selected = config_.monitors.empty()
                            ? m.primary
                            : std::find(config_.monitors.begin(), config_.monitors.end(),
                                        utf8(m.device)) != config_.monitors.end();
        if (selected)
            SendMessageW(control(MonitorList), LB_SETSEL, TRUE, i);
    }
    if (resume) {
        enabled_ = true;
        start();
    }
    refresh_controls();
}
void App::start() {
    if (emergency_)
        throw std::runtime_error(
            "Emergency disable active; release Ctrl+Alt+Home and enable again");
    if (selection(control(ModeSelect)) == 1)
        throw std::runtime_error("Game shader mode is external. Install the supplied .fx file "
                                 "through ReShade; desktop filtering stays off.");
    std::vector<Monitor> chosen;
    for (size_t i = 0; i < monitors_.size(); ++i)
        if (config_.allMonitors || SendMessageW(control(MonitorList), LB_GETSEL, i, 0) > 0)
            chosen.push_back(monitors_[i]);
    if (chosen.empty())
        throw std::runtime_error("Select at least one connected display");
    try {
        for (const auto &m : chosen) {
            if (m.hdr)
                throw std::runtime_error(
                    "A selected display uses HDR. Turn off HDR before using this SDR simulation.");
            HWND h = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE |
                                         WS_EX_LAYERED | WS_EX_TRANSPARENT,
                                     L"CVS.Overlay.1", L"CVS filtered display", WS_POPUP,
                                     m.bounds.left, m.bounds.top, m.bounds.right - m.bounds.left,
                                     m.bounds.bottom - m.bounds.top, nullptr, nullptr, instance_,
                                     window_);
            if (!h)
                throw std::runtime_error("Cannot create display overlay");
            overlays_.push_back(h);
            if (!SetLayeredWindowAttributes(h, 0, 255, LWA_ALPHA))
                throw std::runtime_error("Cannot make overlay click-through");
            DWORD affinity{};
            if (!SetWindowDisplayAffinity(h, WDA_EXCLUDEFROMCAPTURE) ||
                !GetWindowDisplayAffinity(h, &affinity) || affinity != WDA_EXCLUDEFROMCAPTURE)
                throw std::runtime_error("Overlay capture exclusion failed. Filtering refused to "
                                         "prevent recursive capture.");
            {
                std::lock_guard lock(safetyMutex_);
                safetyWindows_ = overlays_;
            }
            captures_.push_back(
                std::make_unique<Capture>(m, h, config_.settings, config_.fpsLimit));
        }
        enabled_ = true;
        paused_ = false;
        overlayRecoveries_ = 0;
        notice_.clear();
        log("Desktop filtering enabled on " + std::to_string(chosen.size()) + " display(s)");
    } catch (...) {
        stop();
        throw;
    }
}
void App::stop() {
    {
        std::lock_guard lock(safetyMutex_);
        safetyWindows_.clear();
    }
    // Hide before joining GPU workers so any driver delay cannot leave an opaque screen.
    for (auto h : overlays_)
        ShowWindow(h, SW_HIDE);
    captures_.clear();
    for (auto h : overlays_)
        DestroyWindow(h);
    overlays_.clear();
    enabled_ = false;
    paused_ = false;
    if (window_)
        SetWindowPos(window_, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
void App::toggle() {
    if (enabled_)
        stop();
    else {
        emergency_ = false;
        start();
    }
    refresh_controls();
}
void App::pause() {
    if (!enabled_)
        return;
    if (paused_) {
        emergency_ = false;
        start();
    } else {
        stop();
        enabled_ = true;
        paused_ = true;
    }
    refresh_controls();
}
void App::update_transform() {
    validate(config_.settings);
    for (auto &c : captures_)
        c->update(config_.settings, config_.fpsLimit);
    dirty_ = true;
    refresh_controls();
}
void App::persist() {
    if (smoke_)
        return;
    try {
        save_config(configPath_, config_);
        dirty_ = false;
    } catch (const std::exception &e) {
        notice_ = L"Settings could not be saved: " + wide(e.what());
        log(e.what());
    }
}
void App::select_preset(int index) {
    config_.settings.preset = static_cast<Preset>(index);
    config_.settings.severity = presets()[index].severity;
    update_transform();
    persist();
}
void App::next_preset(int delta) {
    select_preset((static_cast<int>(config_.settings.preset) + delta + 15) % 15);
}
void App::register_hotkeys() {
    hotkeyStatus_.clear();
    for (int i = 0; i < 5; ++i) {
        UnregisterHotKey(window_, i + 1);
        auto h = config_.hotkeys[i];
        registered_[i] = RegisterHotKey(window_, i + 1, h.modifiers | MOD_NOREPEAT, h.key) != FALSE;
        if (!registered_[i])
            hotkeyStatus_ += L"Shortcut " + std::to_wstring(i + 1) +
                             L" unavailable (conflict/reserved, error " +
                             std::to_wstring(GetLastError()) + L").\r\n";
    }
    UnregisterHotKey(window_, 10);
    emergencyRegistered_ =
        RegisterHotKey(window_, 10, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, VK_HOME) != FALSE;
    if (!emergencyRegistered_)
        hotkeyStatus_ +=
            L"Emergency key registration conflicted; independent key polling remains active.\r\n";
    if (hotkeyStatus_.empty())
        hotkeyStatus_ = L"All shortcuts registered.";
    set(control(HotkeyStatus), hotkeyStatus_);
    log(utf8(hotkeyStatus_));
}
void App::apply_hotkeys() {
    auto original = config_.hotkeys, newKeys = original;
    for (int i = 0; i < 5; ++i) {
        auto v = SendMessageW(control(Hotkey0 + i), HKM_GETHOTKEY, 0, 0);
        BYTE mods = HIBYTE(v);
        newKeys[i] = {static_cast<unsigned>(((mods & HOTKEYF_ALT) ? MOD_ALT : 0) |
                                            ((mods & HOTKEYF_CONTROL) ? MOD_CONTROL : 0) |
                                            ((mods & HOTKEYF_SHIFT) ? MOD_SHIFT : 0)),
                      LOBYTE(v)};
    }
    validate_hotkeys(newKeys);
    config_.hotkeys = newKeys;
    register_hotkeys();
    if (std::any_of(registered_.begin(), registered_.end(), [](bool b) { return !b; })) {
        auto errors = hotkeyStatus_;
        config_.hotkeys = original;
        register_hotkeys();
        refresh_controls();
        throw std::runtime_error("New shortcuts were not applied; previous bindings restored. " +
                                 utf8(errors));
    }
    persist();
    refresh_controls();
}
void App::tray(bool addIcon) {
    NOTIFYICONDATAW n{sizeof(n)};
    n.hWnd = window_;
    n.uID = 1;
    n.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    n.uCallbackMessage = trayMessage;
    n.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(n.szTip, L"Color Vision Simulator — Ctrl+Alt+Home disables");
    if (addIcon) {
        trayAdded_ = Shell_NotifyIconW(NIM_ADD, &n) != FALSE;
        if (trayAdded_) {
            n.uVersion = NOTIFYICON_VERSION_4;
            Shell_NotifyIconW(NIM_SETVERSION, &n);
        }
    } else if (trayAdded_) {
        Shell_NotifyIconW(NIM_DELETE, &n);
        trayAdded_ = false;
    }
}
void App::tray_menu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | (enabled_ ? MF_CHECKED : 0), TrayToggle, L"Enable / disable");
    AppendMenuW(menu, MF_STRING, TrayPause, paused_ ? L"Resume" : L"Pause");
    HMENU choices = CreatePopupMenu();
    for (int i = 0; i < 15; ++i) {
        auto s = wide(presets()[i].name);
        AppendMenuW(choices,
                    MF_STRING | (i == static_cast<int>(config_.settings.preset) ? MF_CHECKED : 0),
                    TrayPreset + i, s.c_str());
    }
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(choices), L"Preset");
    AppendMenuW(menu, MF_STRING, TrayNext, L"Next preset");
    AppendMenuW(menu, MF_STRING, TrayPrevious, L"Previous preset");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, TrayOpen, L"Open settings");
    AppendMenuW(menu, MF_STRING, TrayExit, L"Exit safely");
    POINT p{};
    GetCursorPos(&p);
    SetForegroundWindow(window_);
    int id = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, p.x, p.y, 0, window_, nullptr);
    DestroyMenu(menu);
    PostMessageW(window_, WM_NULL, 0, 0);
    if (id)
        command(id, 0);
}
void App::show() {
    ShowWindow(window_, SW_RESTORE);
    if (enabled_ && !paused_)
        SetWindowPos(window_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetForegroundWindow(window_);
}
void App::timer() {
    if (emergency_ && enabled_) {
        stop();
        notice_ = L"Emergency disable: all overlays stopped.";
        log("Emergency disable");
        refresh_controls();
    }
    std::wstring details;
    double fps = 0, gpu = 0, age = 0;
    int gpuCount = 0, ageCount = 0;
    bool fault = false;
    for (size_t i = 0; i < captures_.size(); ++i) {
        auto s = captures_[i]->stats();
        fps += s.fps;
        if (s.gpuMs >= 0) {
            gpu += s.gpuMs;
            ++gpuCount;
        }
        if (s.frameAgeMs >= 0) {
            age += s.frameAgeMs;
            ++ageCount;
        }
        details += L"Display " + std::to_wstring(i + 1) + L": " + fixed(s.fps) +
                   L" FPS | GPU draw " + (s.gpuMs >= 0 ? fixed(s.gpuMs, 3) + L" ms" : L"N/A") +
                   L" | frame age " + (s.frameAgeMs >= 0 ? fixed(s.frameAgeMs) + L" ms" : L"N/A") +
                   L" | " + wide(s.status) + L"\r\n";
        if (s.failed || std::chrono::steady_clock::now() - s.heartbeat > std::chrono::seconds(5)) {
            fault = true;
            notice_ = L"Filtering stopped: " + wide(s.status);
            log(utf8(notice_));
        }
    }
    PROCESS_MEMORY_COUNTERS_EX memory{};
    memory.cb = sizeof(memory);
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory),
                         sizeof(memory));
    details += L"Process private memory: " +
               fixed(static_cast<double>(memory.PrivateUsage) / (1024 * 1024)) +
               L" MiB. Frame age = capture to submission; not end-to-end latency.";
    if (!captures_.empty())
        details += L"\r\nCapture: Windows Graphics Capture. Mode: D3D11 desktop overlay. SDR sRGB.";
    set(control(Diagnostics), details);
    if (fault) {
        stop();
        refresh_controls();
    } else if (enabled_ && !paused_) {
        // The existing 100 ms UI timer also handles games that change their
        // topmost order without a new activation event. Never take keyboard focus.
        for (auto h : overlays_)
            if (maintain_overlay_order(h, window_)) {
                if (overlayRecoveries_++ == 0)
                    log("Restored overlay above foreground window without changing focus");
            }
    }
    std::wstring status =
        enabled_ ? (paused_ ? L"Paused. Desktop is unfiltered."
                            : L"Filtering " + std::to_wstring(captures_.size()) + L" display(s). " +
                                  (captures_.empty()
                                       ? L""
                                       : fixed(fps / captures_.size()) + L" average FPS. "))
                 : L"Filtering is off. ";
    if (enabled_ && !paused_)
        status += L"GPU " + (gpuCount ? fixed(gpu / gpuCount, 3) + L" ms" : L"N/A") +
                  L" | frame age " + (ageCount ? fixed(age / ageCount) + L" ms" : L"N/A");
    if (overlayRecoveries_ && enabled_ && !paused_)
        status += L" | overlay order recovered " + std::to_wstring(overlayRecoveries_) + L" times";
    if (!notice_.empty())
        status += L" " + notice_;
    set(control(Status), status);
}
void App::draw_preview(const DRAWITEMSTRUCT *d) {
    auto t = make_transform(config_.settings);
    FillRect(d->hDC, &d->rcItem, GetSysColorBrush(COLOR_WINDOW));
    SetBkMode(d->hDC, TRANSPARENT);
    SetTextColor(d->hDC, GetSysColor(COLOR_WINDOWTEXT));
    auto old = SelectObject(d->hDC, font_);
    int scale = dpi_;
    int left = MulDiv(94, scale, 96);
    int width = d->rcItem.right - left - 8;
    int row = MulDiv(36, scale, 96);
    TextOutW(d->hDC, 4, 10, L"Original", 8);
    TextOutW(d->hDC, 4, row + 22, L"Simulated", 9);
    constexpr Vec3 colors[] = {{1, 0, 0},         {0, 1, 0},         {0, 0, 1},         {1, 1, 0},
                               {0, 1, 1},         {1, 0, 1},         {1, .5f, 0},       {.5f, 0, 1},
                               {.9f, .55f, .45f}, {.45f, .7f, .25f}, {.1f, .55f, .85f}, {0, 0, 0},
                               {.2f, .2f, .2f},   {.5f, .5f, .5f},   {.8f, .8f, .8f},   {1, 1, 1}};
    for (int r = 0; r < 2; ++r)
        for (int i = 0; i < 16; ++i) {
            Vec3 c = r ? transform_pixel(colors[i], t) : colors[i];
            auto brush = CreateSolidBrush(RGB(static_cast<BYTE>(std::lround(c.x * 255)),
                                              static_cast<BYTE>(std::lround(c.y * 255)),
                                              static_cast<BYTE>(std::lround(c.z * 255))));
            RECT box{left + i * width / 16, 8 + r * (row + 12), left + (i + 1) * width / 16 - 1,
                     8 + r * (row + 12) + row};
            FillRect(d->hDC, &box, brush);
            DeleteObject(brush);
        }
    SelectObject(d->hDC, old);
}
static std::filesystem::path choose_file(HWND owner, bool save) {
    wchar_t file[32768] = L"cvs-presets.json";
    OPENFILENAMEW o{sizeof(o)};
    o.hwndOwner = owner;
    o.lpstrFilter = L"JSON presets\0*.json\0All files\0*.*\0\0";
    o.lpstrFile = file;
    o.nMaxFile = 32768;
    o.lpstrDefExt = L"json";
    o.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
              (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    return (save ? GetSaveFileNameW(&o) : GetOpenFileNameW(&o)) ? std::filesystem::path(file)
                                                                : std::filesystem::path{};
}
void App::import_presets() {
    auto path = choose_file(window_, false);
    if (path.empty())
        return;
    auto imported = load_config(path);
    Config merged = config_;
    merged.settings = imported.settings;
    for (auto p : imported.saved) {
        std::string base = p.name;
        int suffix = 1;
        while (std::any_of(merged.saved.begin(), merged.saved.end(),
                           [&](const auto &a) { return a.name == p.name; }))
            p.name = base + " (" + std::to_string(++suffix) + ")";
        merged.saved.push_back(p);
    }
    serialize(merged);
    config_ = std::move(merged);
    refresh_saved();
    update_transform();
    persist();
    notice_ = L"Preset file imported.";
}
void App::export_presets() {
    auto path = choose_file(window_, true);
    if (path.empty())
        return;
    Config out;
    out.settings = config_.settings;
    out.saved = config_.saved;
    save_config(path, out);
    notice_ = L"Presets exported.";
}
void App::save_preset() {
    auto name = utf8(text(control(PresetName)));
    if (name.empty())
        throw std::runtime_error("Enter a preset name");
    Config next = config_;
    auto it = std::find_if(next.saved.begin(), next.saved.end(),
                           [&](const auto &p) { return p.name == name; });
    if (it == next.saved.end())
        next.saved.push_back({name, config_.settings});
    else
        it->settings = config_.settings;
    serialize(next);
    config_ = std::move(next);
    refresh_saved();
    persist();
    notice_ = L"Preset saved.";
}
void App::apply_matrix() {
    auto s = config_.settings;
    for (int i = 0; i < 9; ++i) {
        auto str = text(control(Matrix0 + i));
        size_t n{};
        float value = std::stof(str, &n);
        if (n != str.size())
            throw std::runtime_error("Matrix fields must contain only a number");
        s.custom.v[i] = value;
    }
    validate(s);
    config_.settings = s;
    update_transform();
    persist();
}
void App::command(int id, int notification) {
    if (loading_)
        return;
    if (id >= TrayPreset && id < TrayPreset + 15) {
        select_preset(id - TrayPreset);
        return;
    }
    switch (id) {
    case Enable:
    case TrayToggle:
        toggle();
        break;
    case Pause:
    case TrayPause:
        pause();
        break;
    case ToTray:
        if (trayAdded_)
            ShowWindow(window_, SW_HIDE);
        else
            throw std::runtime_error("System tray icon is unavailable; keeping settings visible");
        break;
    case TrayOpen:
        show();
        break;
    case TrayExit:
        stop();
        persist();
        DestroyWindow(window_);
        break;
    case TrayNext:
        next_preset(1);
        break;
    case TrayPrevious:
        next_preset(-1);
        break;
    case Search:
        if (notification == EN_CHANGE)
            filter_presets();
        break;
    case PresetList:
        if (notification == LBN_SELCHANGE) {
            int i = list_selection(control(PresetList));
            if (i >= 0 && i < static_cast<int>(visiblePresets_.size()))
                select_preset(visiblePresets_[i]);
        }
        break;
    case ModelSelect:
        if (notification == CBN_SELCHANGE) {
            config_.settings.model = static_cast<Model>(selection(control(ModelSelect)));
            update_transform();
            persist();
        }
        break;
    case ModeSelect:
        if (notification == CBN_SELCHANGE) {
            stop();
            if (selection(control(ModeSelect)) == 1)
                notice_ = L"Use optional-integrations/game-shaders/ColorVisionSimulator.fx with a "
                          L"permitted ReShade installation. See README.";
            refresh_controls();
        }
        break;
    case AllMonitors:
        config_.allMonitors = checked(control(AllMonitors));
        [[fallthrough]];
    case MonitorList:
        if (id == AllMonitors || notification == LBN_SELCHANGE) {
            config_.monitors.clear();
            for (size_t i = 0; i < monitors_.size(); ++i)
                if (SendMessageW(control(MonitorList), LB_GETSEL, i, 0) > 0)
                    config_.monitors.push_back(utf8(monitors_[i].device));
            bool resume = enabled_ && !paused_;
            stop();
            if (resume)
                start();
            refresh_controls();
            persist();
        }
        break;
    case RefreshMonitors:
        reload_monitors();
        break;
    case ConeSurvivor:
        if (notification == CBN_SELCHANGE) {
            config_.settings.survivingCone = selection(control(ConeSurvivor));
            update_transform();
            persist();
        }
        break;
    case MatrixApply:
        apply_matrix();
        break;
    case ResetArtistic:
        config_.settings.saturation = config_.settings.contrast = 1;
        config_.settings.brightness = 0;
        update_transform();
        persist();
        break;
    case SavePreset:
        save_preset();
        break;
    case LoadPreset: {
        int i = list_selection(control(SavedList));
        if (i >= 0 && i < static_cast<int>(config_.saved.size())) {
            config_.settings = config_.saved[i].settings;
            set(control(PresetName), wide(config_.saved[i].name));
            update_transform();
            persist();
        }
    } break;
    case DeletePreset: {
        int i = list_selection(control(SavedList));
        if (i >= 0 && i < static_cast<int>(config_.saved.size())) {
            config_.saved.erase(config_.saved.begin() + i);
            refresh_saved();
            persist();
        }
    } break;
    case ImportPreset:
        import_presets();
        break;
    case ExportPreset:
        export_presets();
        break;
    case ApplyHotkeys:
        apply_hotkeys();
        break;
    case MinimizeTray:
        config_.minimizeToTray = checked(control(MinimizeTray));
        persist();
        break;
    case FpsLimit:
        if (notification == EN_KILLFOCUS) {
            auto s = text(control(FpsLimit));
            size_t n;
            int limit = std::stoi(s, &n);
            if (n != s.size() || limit < 15 || limit > 240)
                throw std::runtime_error("FPS limit must be 15..240");
            config_.fpsLimit = limit;
            update_transform();
            persist();
        }
        break;
    case OpenLogs:
        std::filesystem::create_directories(logPath_.parent_path());
        ShellExecuteW(window_, L"open", logPath_.parent_path().c_str(), nullptr, nullptr,
                      SW_SHOWNORMAL);
        break;
    default:
        break;
    }
}
LRESULT CALLBACK App::procedure(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    App *app = reinterpret_cast<App *>(GetWindowLongPtrW(w, GWLP_USERDATA));
    if (m == WM_NCCREATE) {
        app = static_cast<App *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
        app->window_ = w;
        SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    if (!app)
        return DefWindowProcW(w, m, wp, lp);
    try {
        return app->message(m, wp, lp);
    } catch (const std::exception &e) {
        app->stop();
        app->refresh_controls();
        app->report(e);
        if (app->smoke_) {
            app->smokeFailed_ = true;
            DestroyWindow(w);
        }
        return 0;
    } catch (...) {
        app->stop();
        if (app->smoke_) {
            app->smokeFailed_ = true;
            DestroyWindow(w);
        }
        return 0;
    }
}
LRESULT App::message(UINT m, WPARAM wp, LPARAM lp) {
    if (taskbarCreated_ && m == taskbarCreated_) {
        trayAdded_ = false;
        tray(true);
        return 0;
    }
    switch (m) {
    case WM_COMMAND:
        command(LOWORD(wp), HIWORD(wp));
        return 0;
    case WM_HSCROLL: {
        int id = GetDlgCtrlID(reinterpret_cast<HWND>(lp));
        float v =
            static_cast<float>(SendMessageW(reinterpret_cast<HWND>(lp), TBM_GETPOS, 0, 0)) / 100;
        if (id == Severity)
            config_.settings.severity = v;
        else if (id >= ConeL && id <= ConeS)
            config_.settings.cones[id - ConeL] = v;
        else if (id == Saturation)
            config_.settings.saturation = v;
        else if (id == Brightness)
            config_.settings.brightness = v;
        else if (id == Contrast)
            config_.settings.contrast = v;
        update_transform();
        if (LOWORD(wp) == TB_ENDTRACK)
            persist();
        return 0;
    }
    case WM_NOTIFY:
        if (reinterpret_cast<NMHDR *>(lp)->idFrom == Tab &&
            reinterpret_cast<NMHDR *>(lp)->code == TCN_SELCHANGE) {
            tab_ = TabCtrl_GetCurSel(control(Tab));
            scroll_ = 0;
            layout();
        }
        return 0;
    case WM_DRAWITEM:
        if (wp == Preview) {
            draw_preview(reinterpret_cast<DRAWITEMSTRUCT *>(lp));
            return TRUE;
        }
        break;
    case WM_HOTKEY:
        if (wp == 10) {
            emergency_ = true;
            stop();
            notice_ = L"Emergency disable.";
            refresh_controls();
        } else if (wp == 1)
            toggle();
        else if (wp == 2)
            next_preset(1);
        else if (wp == 3)
            next_preset(-1);
        else if (wp == 4 || wp == 5) {
            config_.settings.severity =
                std::clamp(config_.settings.severity + (wp == 4 ? .05f : -.05f), 0.f, 1.f);
            update_transform();
            persist();
        }
        return 0;
    case emergencyMessage:
        emergency_ = true;
        stop();
        notice_ = L"Emergency disable.";
        refresh_controls();
        return 0;
    case openMessage:
        show();
        return 0;
    case trayMessage:
        if (LOWORD(lp) == WM_CONTEXTMENU || LOWORD(lp) == WM_RBUTTONUP)
            tray_menu();
        else if (LOWORD(lp) == NIN_SELECT || LOWORD(lp) == WM_LBUTTONDBLCLK)
            show();
        return 0;
    case WM_TIMER:
        if (wp == 2 && smoke_)
            smoke_step();
        else if (wp == 3) {
            KillTimer(window_, 3);
            reload_monitors();
        } else
            timer();
        return 0;
    case WM_DISPLAYCHANGE:
        for (auto h : overlays_)
            ShowWindow(h, SW_HIDE);
        SetTimer(window_, 3, 500, nullptr);
        return 0;
    case WM_POWERBROADCAST:
        if (wp == PBT_APMSUSPEND) {
            stop();
            refresh_controls();
        } else if (wp == PBT_APMRESUMEAUTOMATIC)
            SetTimer(window_, 3, 500, nullptr);
        return TRUE;
    case WM_DPICHANGED:
        dpi_ = HIWORD(wp);
        {
            auto r = reinterpret_cast<RECT *>(lp);
            SetWindowPos(window_, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        layout();
        return 0;
    case WM_SIZE:
        layout();
        return 0;
    case WM_GETMINMAXINFO: {
        auto p = reinterpret_cast<MINMAXINFO *>(lp);
        p->ptMinTrackSize.x = MulDiv(830, dpi_, 96);
        p->ptMinTrackSize.y = MulDiv(360, dpi_, 96);
        return 0;
    }
    case WM_VSCROLL: {
        SCROLLINFO s{sizeof(s), SIF_ALL};
        GetScrollInfo(window_, SB_VERT, &s);
        switch (LOWORD(wp)) {
        case SB_LINEUP:
            scroll_ -= 32;
            break;
        case SB_LINEDOWN:
            scroll_ += 32;
            break;
        case SB_PAGEUP:
            scroll_ -= s.nPage;
            break;
        case SB_PAGEDOWN:
            scroll_ += s.nPage;
            break;
        case SB_THUMBTRACK:
            scroll_ = s.nTrackPos;
            break;
        default:
            break;
        }
        layout();
        return 0;
    }
    case WM_MOUSEWHEEL:
        scroll_ -= GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA * 48;
        layout();
        return 0;
    case WM_CLOSE:
        if (config_.minimizeToTray && trayAdded_ && !smoke_) {
            persist();
            ShowWindow(window_, SW_HIDE);
        } else {
            stop();
            persist();
            DestroyWindow(window_);
        }
        return 0;
    case WM_QUERYENDSESSION:
        stop();
        persist();
        return TRUE;
    case WM_ENDSESSION:
        if (wp) {
            stop();
            tray(false);
        }
        return 0;
    case WM_DESTROY:
        tray(false);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window_, m, wp, lp);
}
void App::smoke_step() {
    if (smokeStage_ == 0) {
        if (!trayAdded_ || !registered_[0] || !registered_[1] || !emergencyRegistered_)
            throw std::runtime_error("Smoke: required tray/hotkey registration failed");
        if (!enabled_ || captures_.empty() || !captures_.front()->stats().running)
            throw std::runtime_error("Smoke: real Ctrl+Alt+F8 did not enable capture");
        log("PASS real global F8 enables capture; tray icon and emergency shortcut registered");
        send_test_hotkey(VK_F8);
        send_test_hotkey(VK_F9);
        smokeStage_ = 1;
        SetTimer(window_, 2, 600, nullptr);
        return;
    }
    if (smokeStage_ == 1) {
        if (enabled_ || config_.settings.preset != Preset::Deuteranomaly)
            throw std::runtime_error("Smoke: real toggle/next hotkey failed");
        SendMessageW(control(Severity), TBM_SETPOS, TRUE, 75);
        SendMessageW(window_, WM_HSCROLL, TB_ENDTRACK, reinterpret_cast<LPARAM>(control(Severity)));
        if (std::abs(config_.settings.severity - .75f) > 1e-6f)
            throw std::runtime_error("Smoke: severity control failed");
        set(control(PresetName), L"Smoke preset");
        SendMessageW(window_, WM_COMMAND, SavePreset, 0);
        if (config_.saved.size() != 1)
            throw std::runtime_error("Smoke: preset manager save failed");
        config_.settings.severity = .2f;
        SendMessageW(control(SavedList), LB_SETCURSEL, 0, 0);
        SendMessageW(window_, WM_COMMAND, LoadPreset, 0);
        if (std::abs(config_.settings.severity - .75f) > 1e-6f)
            throw std::runtime_error("Smoke: preset manager load failed");
        log("PASS real global toggle/next, severity control and preset-manager save/load");
        // Reserve a chord on another window and prove customization rolls back atomically.
        HWND competitor = CreateWindowExW(0, L"STATIC", L"CVS hotkey conflict test", 0, 0, 0, 0, 0,
                                          HWND_MESSAGE, nullptr, instance_, nullptr);
        if (!competitor || !RegisterHotKey(competitor, 1, MOD_CONTROL | MOD_ALT, VK_F6)) {
            if (competitor)
                DestroyWindow(competitor);
            throw std::runtime_error("Smoke: cannot establish hotkey conflict fixture");
        }
        const auto previous = config_.hotkeys;
        SendMessageW(control(Hotkey0), HKM_SETHOTKEY,
                     MAKEWORD(VK_F6, HOTKEYF_CONTROL | HOTKEYF_ALT), 0);
        SendMessageW(control(Hotkey0 + 4), HKM_SETHOTKEY,
                     MAKEWORD(VK_NEXT, HOTKEYF_CONTROL | HOTKEYF_ALT), 0);
        bool rejected = false;
        try {
            apply_hotkeys();
        } catch (const std::exception &e) {
            rejected =
                std::string(e.what()).find("previous bindings restored") != std::string::npos;
        }
        UnregisterHotKey(competitor, 1);
        DestroyWindow(competitor);
        for (size_t i = 0; i < previous.size(); ++i)
            if (previous[i].key != config_.hotkeys[i].key ||
                previous[i].modifiers != config_.hotkeys[i].modifiers)
                throw std::runtime_error("Smoke: hotkey conflict changed saved bindings");
        if (!rejected || !registered_[0] || !registered_[1])
            throw std::runtime_error("Smoke: hotkey conflict was not rejected and rolled back");
        log("PASS occupied global shortcut rejected; previous bindings restored");
        send_test_hotkey(VK_F8);
        smokeStage_ = 2;
        SetTimer(window_, 2, 1000, nullptr);
        return;
    }
    if (smokeStage_ == 2) {
        if (!enabled_ || captures_.empty() || !captures_.front()->stats().running)
            throw std::runtime_error("Smoke: re-enable failed");
        // Reproduce a game becoming foreground/topmost after capture has started.
        // Minimize CVS too: owned overlays used to disappear with their owner.
        ShowWindow(window_, SW_MINIMIZE);
        RECT bounds{};
        GetWindowRect(overlays_.front(), &bounds);
        smokeGame_ = CreateWindowExW(WS_EX_TOPMOST, L"STATIC", L"CVS focused game fixture",
                                     WS_OVERLAPPEDWINDOW, bounds.left + 80, bounds.top + 80, 320,
                                     240, nullptr, nullptr, instance_, nullptr);
        if (!smokeGame_)
            throw std::runtime_error("Smoke: cannot create game fixture");
        ShowWindow(smokeGame_, SW_SHOW);
        SetWindowPos(smokeGame_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(smokeGame_);
        if (GetForegroundWindow() != smokeGame_ ||
            !test_window_above(smokeGame_, overlays_.front()))
            throw std::runtime_error("Smoke: game did not reproduce foreground occlusion");
        log("Fixture reproduced: focused topmost game covers the active filter");
        smokeStage_ = 3;
        SetTimer(window_, 2, 600, nullptr);
        return;
    }
    if (smokeStage_ == 3 || smokeStage_ == 4) {
        if (!enabled_ || overlays_.empty() || !IsWindowVisible(overlays_.front()) ||
            !test_window_above(overlays_.front(), smokeGame_) ||
            GetForegroundWindow() != smokeGame_ || !captures_.front()->stats().running)
            throw std::runtime_error("Smoke: focused game not covered without stealing focus");
        if (smokeStage_ == 3) {
            log("PASS windowed foreground recovery while CVS is minimized; game keeps focus");
            // The game changes to borderless and raises itself again while focused.
            RECT bounds{};
            GetWindowRect(overlays_.front(), &bounds);
            SetWindowLongPtrW(smokeGame_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(smokeGame_, HWND_TOPMOST, bounds.left, bounds.top,
                         bounds.right - bounds.left, bounds.bottom - bounds.top,
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            if (!test_window_above(smokeGame_, overlays_.front()))
                throw std::runtime_error("Smoke: borderless fixture did not cover filter");
            smokeStage_ = 4;
            return;
        }
        log("PASS repeated borderless foreground recovery; game keeps focus");
        DestroyWindow(smokeGame_);
        smokeGame_ = nullptr;
        show();
        send_test_hotkey(VK_HOME);
        smokeStage_ = 5;
        SetTimer(window_, 2, 600, nullptr);
        return;
    }
    if (enabled_ || !captures_.empty() || !emergency_)
        throw std::runtime_error("Smoke: emergency disable failed");
    log("PASS real Ctrl+Alt+Home removes overlays; safe re-enable/disable lifecycle");
    save_smoke_image();
    KillTimer(window_, 2);
    DestroyWindow(window_);
}
void App::save_smoke_image() {
    SetWindowDisplayAffinity(window_, WDA_NONE);
    show();
    UpdateWindow(window_);
    RECT r{};
    GetWindowRect(window_, &r);
    int width = r.right - r.left, height = r.bottom - r.top;
    HDC screen = GetDC(window_), memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
    auto old = SelectObject(memory, bitmap);
    BOOL ok = PrintWindow(window_, memory, 2);
    SelectObject(memory, old);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
    if (ok)
        ok = GetDIBits(screen, bitmap, 0, height, pixels.data(), &info, DIB_RGB_COLORS) != 0;
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(window_, screen);
    if (!ok)
        throw std::runtime_error("Smoke: GUI screenshot failed");
    BITMAPFILEHEADER header{};
    header.bfType = 0x4d42;
    header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
    header.bfSize = header.bfOffBits + static_cast<DWORD>(pixels.size());
    std::filesystem::create_directories(logPath_.parent_path());
    std::ofstream out(logPath_.parent_path() / L"gui.bmp", std::ios::binary);
    out.write(reinterpret_cast<const char *>(&header), sizeof(header));
    out.write(reinterpret_cast<const char *>(&info.bmiHeader), sizeof(info.bmiHeader));
    out.write(reinterpret_cast<const char *>(pixels.data()),
              static_cast<std::streamsize>(pixels.size()));
    if (!out)
        throw std::runtime_error("Smoke: screenshot write failed");
}
int App::run(int showCommand) {
    SendMessageW(control(ModeSelect), CB_SETCURSEL, 0, 0);
    ShowWindow(window_, showCommand);
    if (smoke_)
        send_test_hotkey(VK_F8);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(window_, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return smokeFailed_ ? 1 : static_cast<int>(msg.wParam);
}
} // namespace cvs
