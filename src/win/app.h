#pragma once
#include "capture.h"
#include "core/config.h"
#include <filesystem>
#include <memory>

namespace cvs {
inline constexpr wchar_t appClass[] = L"CVS.MainWindow.1";
inline constexpr UINT emergencyMessage = WM_APP + 50, openMessage = WM_APP + 51;
std::wstring wide(std::string_view);
std::string utf8(std::wstring_view);
class App {
    struct Control {
        HWND hwnd{};
        int tab{}, x{}, y{}, w{}, h{};
    };
    HINSTANCE instance_{};
    HWND window_{};
    HWND smokeGame_{};
    HFONT font_{};
    UINT taskbarCreated_{};
    int dpi_{96}, tab_{0}, scroll_{};
    bool enabled_{}, paused_{}, smoke_{}, loading_{}, trayAdded_{}, dirty_{},
        emergencyRegistered_{};
    bool smokeFailed_{};
    int smokeStage_{};
    unsigned overlayRecoveries_{};
    Config config_;
    std::filesystem::path configPath_, logPath_;
    std::vector<Control> controls_;
    std::vector<Monitor> monitors_;
    std::vector<int> visiblePresets_;
    std::vector<HWND> overlays_;
    std::vector<std::unique_ptr<Capture>> captures_;
    std::mutex safetyMutex_;
    std::vector<HWND> safetyWindows_;
    std::jthread watchdog_;
    std::atomic_bool emergency_{false};
    std::wstring hotkeyStatus_, notice_;
    std::array<bool, 5> registered_{};
    HWND add(int id, const wchar_t *cls, const wchar_t *text, DWORD style, int tab, int x, int y,
             int w, int h);
    HWND control(int id) const;
    void create_controls();
    void layout();
    void refresh_controls();
    void filter_presets();
    void refresh_saved();
    void reload_monitors();
    void start();
    void stop();
    void toggle();
    void pause();
    void update_transform();
    void persist();
    void log(const std::string &);
    void report(const std::exception &);
    void register_hotkeys();
    void apply_hotkeys();
    void next_preset(int);
    void select_preset(int);
    void tray(bool add);
    void tray_menu();
    void show();
    void timer();
    void command(int, int);
    void draw_preview(const DRAWITEMSTRUCT *);
    void import_presets();
    void export_presets();
    void save_preset();
    void apply_matrix();
    void smoke_step();
    void save_smoke_image();
    LRESULT message(UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK procedure(HWND, UINT, WPARAM, LPARAM);

  public:
    App(HINSTANCE, bool smoke, bool safe);
    ~App();
    int run(int showCommand);
};
} // namespace cvs
