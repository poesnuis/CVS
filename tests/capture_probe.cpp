// Exercises capture exclusion, shader output and click-through in an actual Windows
// session. Missing WGC/monitor capability is an explicit CTest skip, never a pass.
#include "win/capture.h"
#include "win/overlay.h"
#include <cmath>
#include <d3d9.h>
#include <dwmapi.h>
#include <iostream>
#include <string>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>

static int paintColor = 0, clicks = 0;
static LRESULT CALLBACK source_proc(HWND w, UINT m, WPARAM p, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps{};
        auto dc = BeginPaint(w, &ps);
        RECT r{};
        GetClientRect(w, &r);
        COLORREF colors[]{RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255)};
        auto b = CreateSolidBrush(colors[paintColor]);
        FillRect(dc, &r, b);
        DeleteObject(b);
        EndPaint(w, &ps);
        return 0;
    }
    if (m == WM_LBUTTONDOWN) {
        ++clicks;
        return 0;
    }
    return DefWindowProcW(w, m, p, l);
}
static bool above(HWND first, HWND second) {
    for (HWND h = GetTopWindow(nullptr); h; h = GetWindow(h, GW_HWNDNEXT)) {
        if (h == first)
            return true;
        if (h == second)
            return false;
    }
    return false;
}
struct Window {
    HWND h{};
    ~Window() {
        if (h)
            DestroyWindow(h);
    }
};
int main(int argc, char **argv) {
    using namespace cvs;
    const bool useD3d9 = argc > 1 && std::string(argv[1]) == "--d3d9";
    try {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        if (!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported()) {
            std::cout << "SKIP: Windows Graphics Capture is not supported in this runner session\n";
            return 77;
        }
        auto monitors = enumerate_monitors();
        if (monitors.empty()) {
            std::cout << "SKIP: No connected monitor in this runner session\n";
            return 77;
        }
        auto m = monitors.front();
        WNDCLASSW wc{};
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"CVS.ProbeSource";
        wc.lpfnWndProc = source_proc;
        RegisterClassW(&wc);
        const int x = m.bounds.left + 100, y = m.bounds.top + 100;
        Window source{CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, L"CVS known-color test",
                                      WS_POPUP, x, y, 240, 160, nullptr, nullptr, wc.hInstance,
                                      nullptr)};
        if (!source.h)
            throw std::runtime_error("Cannot create source window");
        ShowWindow(source.h, SW_SHOWNORMAL);
        UpdateWindow(source.h);
        // A legacy accelerated game-like source, in addition to the GDI desktop
        // test. No game files, injection or changes to another process are used.
        ComPtr<IDirect3D9> d3d9;
        ComPtr<IDirect3DDevice9> gameDevice;
        if (useD3d9) {
            d3d9.Attach(Direct3DCreate9(D3D_SDK_VERSION));
            if (!d3d9)
                throw std::runtime_error("Cannot initialize D3D9 game fixture");
            D3DPRESENT_PARAMETERS pp{};
            pp.Windowed = TRUE;
            pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
            pp.hDeviceWindow = source.h;
            pp.BackBufferWidth = 240;
            pp.BackBufferHeight = 160;
            pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
            check_hr(d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, source.h,
                                        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &gameDevice),
                     "Create windowed D3D9 game fixture");
        }
        auto paint = [&] {
            if (gameDevice) {
                const D3DCOLOR colors[]{D3DCOLOR_XRGB(255, 0, 0), D3DCOLOR_XRGB(0, 255, 0),
                                        D3DCOLOR_XRGB(0, 0, 255)};
                check_hr(gameDevice->Clear(0, nullptr, D3DCLEAR_TARGET, colors[paintColor], 1, 0),
                         "Clear D3D9 game frame");
                check_hr(gameDevice->Present(nullptr, nullptr, nullptr, nullptr),
                         "Present D3D9 game frame");
            } else {
                InvalidateRect(source.h, nullptr, FALSE);
                UpdateWindow(source.h);
            }
        };
        paint();
        DwmFlush();
        wc.lpszClassName = L"CVS.ProbeOverlay";
        wc.lpfnWndProc = overlay_procedure;
        RegisterClassW(&wc);
        Window overlay{CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            wc.lpszClassName, L"CVS capture test", WS_POPUP, m.bounds.left, m.bounds.top,
            m.bounds.right - m.bounds.left, m.bounds.bottom - m.bounds.top, nullptr, nullptr,
            wc.hInstance, nullptr)};
        if (!overlay.h)
            throw std::runtime_error("Cannot create probe overlay");
        SetLayeredWindowAttributes(overlay.h, 0, 255, LWA_ALPHA);
        if (!SetWindowDisplayAffinity(overlay.h, WDA_EXCLUDEFROMCAPTURE)) {
            std::cout << "SKIP: Capture exclusion unavailable in runner\n";
            return 77;
        }
        std::atomic_uint samples{0}, colorsSeen{0};
        float maxError = 0;
        auto observe = [&](ID3D11Device *d, ID3D11DeviceContext *ctx, ID3D11Texture2D *raw,
                           ID3D11Texture2D *filtered, const Transform &t) {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            ComPtr<ID3D11Texture2D> staging;
            check_hr(d->CreateTexture2D(&desc, nullptr, &staging), "Probe staging");
            D3D11_BOX box{220, 180, 0,
                          221, 181, 1}; // center of the known source, relative to monitor
            auto read = [&](ID3D11Texture2D *texture) {
                ctx->CopySubresourceRegion(staging.Get(), 0, 0, 0, 0, texture, 0, &box);
                D3D11_MAPPED_SUBRESOURCE map{};
                check_hr(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &map), "Probe readback");
                auto bytes = static_cast<const unsigned char *>(map.pData);
                Vec3 c{bytes[2] / 255.f, bytes[1] / 255.f, bytes[0] / 255.f};
                ctx->Unmap(staging.Get(), 0);
                return c;
            };
            auto original = read(raw), output = read(filtered);
            unsigned color = 0;
            if (original.x > .99f && original.y < .01f && original.z < .01f)
                color = 1;
            if (original.y > .99f && original.x < .01f && original.z < .01f)
                color = 2;
            if (original.z > .99f && original.x < .01f && original.y < .01f)
                color = 4;
            if (!color)
                throw std::runtime_error("Captured source is not an unfiltered primary: possible "
                                         "recursive overlay capture");
            auto expected = transform_pixel(original, t);
            for (int i = 0; i < 3; ++i) {
                float error = std::abs(expected[i] - output[i]);
                maxError = std::max(maxError, error);
                if (error > 2.f / 255)
                    throw std::runtime_error(
                        "Captured desktop shader output differs from reference");
            }
            colorsSeen.fetch_or(color);
            ++samples;
        };
        CaptureStats stats;
        unsigned recoveries = 0;
        {
            Settings settings;
            Capture capture(m, overlay.h, settings, 60, observe);
            auto end = std::chrono::steady_clock::now() + std::chrono::seconds(4);
            int tick = 0;
            while (std::chrono::steady_clock::now() < end) {
                MSG msg{};
                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
                stats = capture.stats();
                if (stats.failed)
                    break;
                if ((tick++ % 5) == 0) {
                    paintColor = (paintColor + 1) % 3;
                    paint();
                }
                if (tick == 20 || tick == 50) {
                    if (!IsWindowVisible(overlay.h))
                        throw std::runtime_error("Filter never became visible");
                    SetWindowPos(source.h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    SetForegroundWindow(source.h);
                    if (GetForegroundWindow() != source.h || !above(source.h, overlay.h))
                        throw std::runtime_error("Game fixture did not reproduce filter occlusion");
                    if (!maintain_overlay_order(overlay.h, nullptr) ||
                        !above(overlay.h, source.h) || GetForegroundWindow() != source.h)
                        throw std::runtime_error("Filter recovery failed or stole game focus");
                    if (maintain_overlay_order(overlay.h, nullptr))
                        throw std::runtime_error("Filter keeps raising itself with correct order");
                    ++recoveries;
                }
                if (tick == 35) {
                    SetCursorPos(x + 120, y + 80);
                    INPUT input[2]{};
                    input[0].type = input[1].type = INPUT_MOUSE;
                    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    if (SendInput(2, input, sizeof(INPUT)) != 2)
                        throw std::runtime_error("Probe could not send test click");
                }
                settings.severity = settings.severity > .5f ? .25f : .75f;
                capture.update(settings, 60);
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
            }
            ShowWindow(overlay.h, SW_HIDE);
            if (maintain_overlay_order(overlay.h, nullptr) || IsWindowVisible(overlay.h))
                throw std::runtime_error("Order maintenance revived a hidden filter");
        }
        if (!stats.running || stats.failed)
            throw std::runtime_error("Capture pipeline failure: " + stats.status);
        if (samples < 8 || colorsSeen != 7)
            throw std::runtime_error("Insufficient captured known-color transitions");
        if (!clicks)
            throw std::runtime_error("Click did not pass through overlay to source window");
        if (recoveries != 2)
            throw std::runtime_error("Missing repeated foreground game recovery checks");
        std::cout
            << "PASS: WGC capture, exclusion across RGB source transitions, D3D11 "
               "output/reference, parameter updates and click-through at "
            << stats.width << "x" << stats.height << "; " << samples
            << " samples; maximum UNORM channel error " << maxError << "; " << clicks
            << " delivered click(s); " << recoveries << " foreground order recoveries with "
            << (useD3d9 ? "D3D9" : "GDI")
            << " source, focus retained and hidden-filter safety. Single runner display only; "
               "not actual FNaF or a hardware benchmark.\n";
        return 0;
    } catch (const winrt::hresult_error &e) {
        std::cerr << "FAIL: " << winrt::to_string(e.message()) << "\n";
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
