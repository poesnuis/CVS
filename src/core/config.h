#pragma once
#include "color.h"
#include <filesystem>
#include <string>
#include <vector>

namespace cvs {
struct NamedPreset { std::string name; Settings settings; };
struct Hotkey { unsigned modifiers{3}, key{}; }; // Win32 MOD_ALT | MOD_CONTROL
struct Config {
    Settings settings;
    std::vector<NamedPreset> saved;
    std::vector<std::string> monitors; // GDI display device names; empty = primary
    bool allMonitors{false};
    bool minimizeToTray{true};
    int fpsLimit{60};
    std::array<Hotkey,5> hotkeys{{{3,0x77},{3,0x78},{3,0x79},{3,0x7a},{3,0x7b}}};
};
std::string serialize(const Config&);
Config deserialize(std::string_view);
Config load_config(const std::filesystem::path&);
void save_config(const std::filesystem::path&,const Config&);
void validate_hotkeys(const std::array<Hotkey,5>&);
}
