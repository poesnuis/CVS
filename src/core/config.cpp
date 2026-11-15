#include "config.h"
#include "json.hpp"
#include <fstream>
#include <set>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace cvs {
using nlohmann::json;
static json settings_json(const Settings& s) {
    validate(s);
    return {{"preset",presets()[static_cast<int>(s.preset)].id},{"model",static_cast<int>(s.model)},
        {"severity",s.severity},{"cone_gains",s.cones},{"surviving_cone",s.survivingCone},
        {"saturation",s.saturation},{"brightness_ev",s.brightness},{"contrast",s.contrast},{"matrix",s.custom.v}};
}
static Settings parse_settings(const json& j) {
    if(!j.is_object()) throw std::invalid_argument("Settings must be an object");
    Settings s; s.preset=preset_from_id(j.at("preset").get<std::string>());
    s.model=static_cast<Model>(j.value("model",0)); s.severity=j.value("severity",1.f);
    s.cones=j.value("cone_gains",s.cones); s.survivingCone=j.value("surviving_cone",0);
    s.saturation=j.value("saturation",1.f); s.brightness=j.value("brightness_ev",0.f);
    s.contrast=j.value("contrast",1.f); s.custom.v=j.value("matrix",identity.v); validate(s); return s;
}
void validate_hotkeys(const std::array<Hotkey,5>& keys) {
    std::set<std::pair<unsigned,unsigned>> seen;
    for(auto h:keys) {
        if((h.modifiers&~15u)!=0||h.modifiers==0||h.key<0x21||h.key>0xfe||h.key==0x7b)
            throw std::invalid_argument("Hotkeys need modifiers and a valid key; F12 is reserved by Windows. Use a different key.");
        if(h.modifiers==3&&h.key==0x24) throw std::invalid_argument("Ctrl+Alt+Home is reserved for emergency disable");
        if(!seen.insert({h.modifiers,h.key}).second) throw std::invalid_argument("Duplicate hotkey");
    }
}
// Windows reserves F12 for the debugger even with modifiers. The documented default
// is attempted at registration, and its failure is visible, never silently ignored.
static void validate_config(const Config& c) {
    validate(c.settings);
    if(c.fpsLimit<15||c.fpsLimit>240) throw std::invalid_argument("Frame limit must be 15..240");
    if(c.saved.size()>128||c.monitors.size()>32) throw std::invalid_argument("Too many presets or monitors");
    std::set<std::string> names;
    for(const auto& p:c.saved) {
        if(p.name.empty()||p.name.size()>128||p.name.find('\0')!=std::string::npos) throw std::invalid_argument("Invalid preset name");
        if(!names.insert(p.name).second) throw std::invalid_argument("Duplicate preset name"); validate(p.settings);
    }
    for(const auto& m:c.monitors) if(m.empty()||m.size()>256) throw std::invalid_argument("Invalid monitor name");
    std::set<std::pair<unsigned,unsigned>> seen;
    for(auto h:c.hotkeys) if(!seen.insert({h.modifiers,h.key}).second) throw std::invalid_argument("Duplicate hotkey");
    // Accept the requested F12 default so its registration failure is visible in the UI.
    for(auto h:c.hotkeys) {
        if((h.modifiers&~15u)||h.modifiers==0||h.key<0x21||h.key>0xfe) throw std::invalid_argument("Invalid hotkey");
        if(h.modifiers==3&&h.key==0x24) throw std::invalid_argument("Emergency hotkey cannot be reassigned");
    }
}
std::string serialize(const Config& c) {
    validate_config(c); json j{{"schema",1},{"settings",settings_json(c.settings)},{"all_monitors",c.allMonitors},
        {"monitors",c.monitors},{"minimize_to_tray",c.minimizeToTray},{"fps_limit",c.fpsLimit}};
    j["presets"]=json::array(); for(const auto& p:c.saved) j["presets"].push_back({{"name",p.name},{"settings",settings_json(p.settings)}});
    j["hotkeys"]=json::array(); for(auto h:c.hotkeys) j["hotkeys"].push_back({{"modifiers",h.modifiers},{"key",h.key}});
    return j.dump(2)+"\n";
}
Config deserialize(std::string_view text) {
    if(text.size()>1024*1024) throw std::invalid_argument("Configuration exceeds 1 MiB");
    int depth=0;
    auto j=json::parse(text.begin(),text.end(),[&](int d,json::parse_event_t,json&) {
        depth=std::max(depth,d); if(depth>24) throw std::invalid_argument("Configuration nesting exceeds limit"); return true;
    });
    if(!j.is_object()||j.at("schema")!=1) throw std::invalid_argument("Unsupported configuration schema");
    Config c;
    if(j.contains("settings")) c.settings=parse_settings(j.at("settings"));
    c.allMonitors=j.value("all_monitors",false); c.monitors=j.value("monitors",c.monitors);
    c.minimizeToTray=j.value("minimize_to_tray",true); c.fpsLimit=j.value("fps_limit",60);
    if(j.contains("presets")) {
        if(!j["presets"].is_array()||j["presets"].size()>128) throw std::invalid_argument("Invalid presets array");
        for(const auto& p:j["presets"]) c.saved.push_back({p.at("name").get<std::string>(),parse_settings(p.at("settings"))});
    }
    if(j.contains("hotkeys")) {
        const auto& h=j["hotkeys"]; if(!h.is_array()||h.size()!=5) throw std::invalid_argument("Exactly five hotkeys required");
        for(size_t i=0;i<5;++i) c.hotkeys[i]={h[i].at("modifiers").get<unsigned>(),h[i].at("key").get<unsigned>()};
    }
    validate_config(c); return c;
}
Config load_config(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>1024*1024) throw std::invalid_argument("Configuration exceeds 1 MiB");
    std::ifstream in(path,std::ios::binary); if(!in) throw std::runtime_error("Cannot open configuration");
    std::string data((std::istreambuf_iterator<char>(in)),{}); return deserialize(data);
}
void save_config(const std::filesystem::path& path,const Config& c) {
    auto data=serialize(c); if(!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    auto tmp=path; tmp+=".tmp";
    { std::ofstream out(tmp,std::ios::binary|std::ios::trunc); if(!out) throw std::runtime_error("Cannot create configuration file");
        out.write(data.data(),static_cast<std::streamsize>(data.size())); out.flush(); if(!out) throw std::runtime_error("Configuration write failed"); }
#ifdef _WIN32
    if(!MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Atomic configuration replace failed");
#else
    std::filesystem::rename(tmp,path);
#endif
}
}
