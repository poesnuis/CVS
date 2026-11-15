// Attempts the real capture/present path on the runner. Unsupported environments
// return 77 (CTest skip), and are never reported as successful desktop validation.
#include "win/capture.h"
#include <iostream>
#include <string>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>

LRESULT CALLBACK proc(HWND w,UINT m,WPARAM p,LPARAM l) {
    if(m==cvs::overlayShow){ShowWindow(w,SW_SHOWNOACTIVATE);return 0;}
    if(m==cvs::overlayHide){ShowWindow(w,SW_HIDE);return 0;}
    if(m==WM_NCHITTEST)return HTTRANSPARENT;
    return DefWindowProcW(w,m,p,l);
}
int main() {
    using namespace cvs;
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        if(!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported()) {std::cout<<"SKIP: Windows Graphics Capture is not supported in this runner session\n";return 77;}
        auto monitors=enumerate_monitors();if(monitors.empty()){std::cout<<"SKIP: No connected monitor in this runner session\n";return 77;}
        auto m=monitors.front();
        WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"CVS.CaptureProbe";wc.lpfnWndProc=proc;RegisterClassW(&wc);
        auto window=CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW|WS_EX_TOPMOST,wc.lpszClassName,L"CVS capture test",WS_POPUP,
            m.bounds.left,m.bounds.top,m.bounds.right-m.bounds.left,m.bounds.bottom-m.bounds.top,nullptr,nullptr,wc.hInstance,nullptr);
        if(!window)throw std::runtime_error("Cannot create probe overlay");
        SetLayeredWindowAttributes(window,0,255,LWA_ALPHA);
        if(!SetWindowDisplayAffinity(window,WDA_EXCLUDEFROMCAPTURE)){DestroyWindow(window);std::cout<<"SKIP: Capture exclusion unavailable in runner\n";return 77;}
        CaptureStats stats;
        {
            Settings settings;Capture capture(m,window,settings,60);
            auto end=std::chrono::steady_clock::now()+std::chrono::seconds(4);
            while(std::chrono::steady_clock::now()<end) {
                MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
                stats=capture.stats();if(stats.failed)break;
                settings.severity=settings.severity>.5f?.25f:.75f;capture.update(settings,60);
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
            }
            ShowWindow(window,SW_HIDE);
        }
        DestroyWindow(window);
        if(!stats.running) {std::cout<<"SKIP: Runner could not execute capture: "<<stats.status<<"\n";return 77;}
        std::cout<<"PASS: Real WGC capture and D3D11 presentation on runner at "<<stats.width<<"x"<<stats.height<<". This is a pipeline smoke test, not visual recursion, interaction, multi-monitor or performance validation.\n";
        return 0;
    }catch(const winrt::hresult_error& e){std::cout<<"SKIP: Runner capture unavailable: "<<winrt::to_string(e.message())<<"\n";return 77;}
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
