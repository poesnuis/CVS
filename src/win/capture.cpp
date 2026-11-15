#include "capture.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <algorithm>
#include <stdexcept>

namespace cvs {
using namespace std::chrono_literals;
namespace wgc=winrt::Windows::Graphics::Capture;
std::vector<Monitor> enumerate_monitors() {
    std::vector<Monitor> result;
    EnumDisplayMonitors(nullptr,nullptr,[](HMONITOR h,HDC,LPRECT,LPARAM p)->BOOL {
        MONITORINFOEXW info{}; info.cbSize=sizeof(info); if(!GetMonitorInfoW(h,&info)) return TRUE;
        Monitor m;m.handle=h;m.bounds=info.rcMonitor;m.device=info.szDevice;m.primary=(info.dwFlags&MONITORINFOF_PRIMARY)!=0;
        DEVMODEW mode{};mode.dmSize=sizeof(mode);if(EnumDisplaySettingsW(info.szDevice,ENUM_CURRENT_SETTINGS,&mode)) m.refresh=std::max(1ul,mode.dmDisplayFrequency);
        m.label=m.device+L"  "+std::to_wstring(m.bounds.right-m.bounds.left)+L" x "+std::to_wstring(m.bounds.bottom-m.bounds.top)+(m.primary?L" (primary)":L"");
        reinterpret_cast<std::vector<Monitor>*>(p)->push_back(m);return TRUE;
    },reinterpret_cast<LPARAM>(&result));
    ComPtr<IDXGIFactory1> factory;check_hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"Enumerate display adapters");
    for(UINT a=0;;++a) {
        ComPtr<IDXGIAdapter1> adapter;if(factory->EnumAdapters1(a,&adapter)==DXGI_ERROR_NOT_FOUND) break;
        DXGI_ADAPTER_DESC1 ad{};adapter->GetDesc1(&ad);
        for(UINT o=0;;++o) {
            ComPtr<IDXGIOutput> output;if(adapter->EnumOutputs(o,&output)==DXGI_ERROR_NOT_FOUND) break;
            DXGI_OUTPUT_DESC od{};if(FAILED(output->GetDesc(&od))) continue;
            for(auto& m:result) if(m.handle==od.Monitor) {
                m.adapter=ad.AdapterLuid;ComPtr<IDXGIOutput6> output6;
                if(SUCCEEDED(output.As(&output6))) {DXGI_OUTPUT_DESC1 d{};if(SUCCEEDED(output6->GetDesc1(&d))) m.hdr=d.ColorSpace==DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;}
                if(m.hdr) m.label+=L" [HDR: unsupported]";
            }
        }
    }
    std::stable_sort(result.begin(),result.end(),[](const Monitor& a,const Monitor& b){return a.primary&&!b.primary;});return result;
}
Capture::Capture(const Monitor& m,HWND overlay,const Settings& s,int fps,FrameObserver observer):monitor_(m),overlay_(overlay),settings_(s),fpsLimit_(fps),observer_(std::move(observer)) {worker_=std::jthread([this](std::stop_token stop){run(stop);});}
Capture::~Capture() {worker_.request_stop(); if(worker_.joinable()) worker_.join();}
void Capture::update(const Settings& s,int fps) {std::lock_guard lock(mutex_);settings_=s;fpsLimit_=fps;++revision_;}
CaptureStats Capture::stats() const {std::lock_guard lock(mutex_);return stats_;}
void Capture::run(std::stop_token stop) {
    try {winrt::init_apartment(winrt::apartment_type::multi_threaded);} catch(...) {std::lock_guard lock(mutex_);stats_.failed=true;stats_.status="Cannot initialize capture apartment";return;}
    for(unsigned attempt=0;attempt<3&&!stop.stop_requested();++attempt) {
        try {session(stop);break;}
        catch(const winrt::hresult_error& e) {std::lock_guard lock(mutex_);stats_.status=winrt::to_string(e.message());}
        catch(const std::exception& e) {std::lock_guard lock(mutex_);stats_.status=e.what();}
        catch(...) {std::lock_guard lock(mutex_);stats_.status="Unknown capture failure";}
        PostMessageW(overlay_,overlayHide,0,0);
        {std::lock_guard lock(mutex_);stats_.running=false;stats_.restarts=attempt+1;stats_.failed=attempt==2;stats_.heartbeat=std::chrono::steady_clock::now();}
        for(int n=0;n<50&&!stop.stop_requested();++n) std::this_thread::sleep_for(10ms);
    }
    PostMessageW(overlay_,overlayHide,0,0);winrt::uninit_apartment();
}
void Capture::session(std::stop_token stop) {
    if(monitor_.hdr) throw std::runtime_error("HDR displays are unsupported; turn off Windows HDR for accurate SDR simulation");
    if(!wgc::GraphicsCaptureSession::IsSupported()) throw std::runtime_error("Windows Graphics Capture is unavailable");
    ComPtr<IDXGIFactory1> factory;check_hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"Create DXGI factory");
    ComPtr<IDXGIAdapter1> adapter;
    for(UINT i=0;;++i) {
        ComPtr<IDXGIAdapter1> candidate;auto hr=factory->EnumAdapters1(i,&candidate);if(hr==DXGI_ERROR_NOT_FOUND) break;check_hr(hr,"Enumerate adapter");
        DXGI_ADAPTER_DESC1 d{};check_hr(candidate->GetDesc1(&d),"Adapter description");
        if(d.AdapterLuid.HighPart==monitor_.adapter.HighPart&&d.AdapterLuid.LowPart==monitor_.adapter.LowPart) {adapter=candidate;break;}
    }
    if(!adapter) throw std::runtime_error("Selected display adapter disconnected");
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL feature;
    D3D_FEATURE_LEVEL levels[]{D3D_FEATURE_LEVEL_11_0};
    check_hr(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,levels,1,D3D11_SDK_VERSION,&device,&feature,&context),"Create hardware D3D11 device");
    // WGC uses the same device from an internal thread; serialize immediate-context access.
    ComPtr<ID3D11Multithread> multithread;check_hr(context.As(&multithread),"D3D11 multithread protection");multithread->SetMultithreadProtected(TRUE);
    Renderer renderer;renderer.initialize(device.Get(),context.Get());
    ComPtr<IDXGIDevice> dxgi;check_hr(device.As(&dxgi),"Get DXGI device");
    winrt::com_ptr<IInspectable> inspectable;winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(),inspectable.put()));
    auto winrtDevice=inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    auto interop=winrt::get_activation_factory<wgc::GraphicsCaptureItem,IGraphicsCaptureItemInterop>();
    wgc::GraphicsCaptureItem item{nullptr};winrt::check_hresult(interop->CreateForMonitor(monitor_.handle,winrt::guid_of<wgc::GraphicsCaptureItem>(),winrt::put_abi(item)));
    auto size=item.Size();if(size.Width<=0||size.Height<=0) throw std::runtime_error("Selected display has no capture area");
    std::atomic_bool closed{false};auto closedToken=item.Closed([&](auto&&,auto&&){closed=true;});
    // This guard outlives session/pool, so callback references stay alive until revocation.
    struct ClosedGuard {wgc::GraphicsCaptureItem& item;winrt::event_token token;~ClosedGuard(){try{item.Closed(token);}catch(...){}}} closedGuard{item,closedToken};
    auto pool=wgc::Direct3D11CaptureFramePool::CreateFreeThreaded(winrtDevice,winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,2,size);
    auto capture=pool.CreateCaptureSession(item);capture.IsCursorCaptureEnabled(false); // native hardware pointer stays responsive
    struct SessionGuard {wgc::GraphicsCaptureSession& c;wgc::Direct3D11CaptureFramePool& p;~SessionGuard(){try{c.Close();p.Close();}catch(...){}}} sessionGuard{capture,pool};
    ComPtr<IDXGIFactory2> factory2;check_hr(factory.As(&factory2),"Get DXGI factory2");
    DXGI_SWAP_CHAIN_DESC1 desc{};desc.Width=size.Width;desc.Height=size.Height;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;
    desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;desc.AlphaMode=DXGI_ALPHA_MODE_IGNORE;
    ComPtr<IDXGISwapChain1> swap;check_hr(factory2->CreateSwapChainForHwnd(device.Get(),overlay_,&desc,nullptr,nullptr,&swap),"Create overlay swap chain");
    factory2->MakeWindowAssociation(overlay_,DXGI_MWA_NO_ALT_ENTER);
    ComPtr<IDXGISwapChain2> swap2;if(SUCCEEDED(swap.As(&swap2))) swap2->SetMaximumFrameLatency(1);
    ComPtr<ID3D11Texture2D> back;check_hr(swap->GetBuffer(0,IID_PPV_ARGS(&back)),"Overlay backbuffer");
    ComPtr<ID3D11RenderTargetView> rtv;check_hr(device->CreateRenderTargetView(back.Get(),nullptr,&rtv),"Overlay render target");
    ComPtr<ID3D11Texture2D> sampled;ComPtr<ID3D11ShaderResourceView> srv;
    capture.StartCapture();
    auto epoch=std::chrono::steady_clock::now(),lastDraw=epoch-1s;unsigned frames=0;bool shown=false;
    uint64_t drawnRevision=~uint64_t{0};int64_t lastTimestamp=0;
    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);
    while(!stop.stop_requested()) {
        const auto now=std::chrono::steady_clock::now();
        Settings settings;int limit;uint64_t revision;
        {std::lock_guard lock(mutex_);settings=settings_;limit=fpsLimit_;revision=revision_;stats_.heartbeat=now;}
        if(closed) throw std::runtime_error("Capture display closed or disconnected");
        check_hr(device->GetDeviceRemovedReason(),"Graphics device removed");
        if(now-lastDraw<std::chrono::duration<double>(1.0/limit)) {std::this_thread::sleep_for(1ms);continue;}
        auto frame=pool.TryGetNextFrame();
        if(!frame&&(!sampled||revision==drawnRevision)) {std::this_thread::sleep_for(2ms);continue;}
        if(frame) {
        // Keep only the newest queued frame to bound queue latency.
        for(int n=0;n<2;++n) {auto newer=pool.TryGetNextFrame();if(!newer) break;frame.Close();frame=std::move(newer);}
        auto content=frame.ContentSize();
        if(content.Width!=size.Width||content.Height!=size.Height) {frame.Close();throw std::runtime_error("Display size changed; rebuilding capture");}
        auto access=frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
        ComPtr<ID3D11Texture2D> texture;winrt::check_hresult(access->GetInterface(IID_PPV_ARGS(&texture)));
        if(!sampled) {
            D3D11_TEXTURE2D_DESC td{};texture->GetDesc(&td);td.BindFlags=D3D11_BIND_SHADER_RESOURCE;td.Usage=D3D11_USAGE_DEFAULT;td.CPUAccessFlags=0;td.MiscFlags=0;
            check_hr(device->CreateTexture2D(&td,nullptr,&sampled),"GPU sampling texture");check_hr(device->CreateShaderResourceView(sampled.Get(),nullptr,&srv),"GPU sampling view");
        }
        context->CopyResource(sampled.Get(),texture.Get()); // GPU-to-GPU only
        lastTimestamp=frame.SystemRelativeTime().count();frame.Close();
        }
        // Re-render the cached GPU frame after parameter changes, even on a static desktop.
        lastDraw=now;drawnRevision=revision;
        auto transform=make_transform(settings);
        renderer.draw(srv.Get(),rtv.Get(),static_cast<unsigned>(size.Width),static_cast<unsigned>(size.Height),transform);
        if(observer_) observer_(device.Get(),context.Get(),sampled.Get(),back.Get(),transform);
        LARGE_INTEGER presentTime{};QueryPerformanceCounter(&presentTime);
        // QPC-based frame age at submission; does not include display scanout or input latency.
        double age=static_cast<double>(presentTime.QuadPart)*1000.0/static_cast<double>(frequency.QuadPart)-static_cast<double>(lastTimestamp)/10000.0;
        auto hr=swap->Present(1,0);check_hr(hr,"Present filtered frame");
        if(!shown) {PostMessageW(overlay_,overlayShow,0,0);shown=true;}
        const auto presentedAt=std::chrono::steady_clock::now();++frames;double seconds=std::chrono::duration<double>(presentedAt-epoch).count();
        {std::lock_guard lock(mutex_);stats_.running=true;stats_.failed=false;stats_.status="Capturing SDR / Windows Graphics Capture / D3D11 overlay";stats_.width=size.Width;stats_.height=size.Height;stats_.gpuMs=renderer.gpu_ms();stats_.frameAgeMs=age>=0&&age<10000?age:-1;
            if(seconds>=1) {stats_.fps=frames/seconds;frames=0;epoch=presentedAt;}}
    }
}
}
