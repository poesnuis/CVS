#pragma once
#include "d3d_renderer.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <functional>

namespace cvs {
inline constexpr UINT overlayShow=WM_APP+40,overlayHide=WM_APP+41;
struct Monitor {
    HMONITOR handle{}; RECT bounds{}; std::wstring device,label; bool primary{},hdr{}; unsigned refresh{60}; LUID adapter{};
};
std::vector<Monitor> enumerate_monitors();
struct CaptureStats {
    double fps{},gpuMs{-1},frameAgeMs{-1};
    unsigned width{},height{},restarts{};
    bool running{},failed{};
    std::string status{"Starting"};
    std::chrono::steady_clock::time_point heartbeat{std::chrono::steady_clock::now()};
};
class Capture {
public:
    // Optional integration-test seam. Empty in the application: no CPU readback.
    using FrameObserver=std::function<void(ID3D11Device*,ID3D11DeviceContext*,ID3D11Texture2D*,ID3D11Texture2D*,const Transform&)>;
private:
    Monitor monitor_; HWND overlay_{};
    std::jthread worker_;
    mutable std::mutex mutex_;
    Settings settings_; int fpsLimit_{};uint64_t revision_{};
    CaptureStats stats_;
    FrameObserver observer_;
    void run(std::stop_token);
    void session(std::stop_token);
public:
    Capture(const Monitor&,HWND,const Settings&,int,FrameObserver observer={});
    ~Capture();
    Capture(const Capture&)=delete;
    void update(const Settings&,int);
    CaptureStats stats() const;
};
}
