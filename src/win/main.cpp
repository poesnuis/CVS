#include "app.h"
#include <commctrl.h>
#include <shellapi.h>
#include <stdexcept>
#include <winternl.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    bool smoke = false, safe = false, disable = false;
    int count{};
    auto args = CommandLineToArgvW(GetCommandLineW(), &count);
    for (int i = 1; i < count; ++i) {
        if (wcscmp(args[i], L"--smoke-test") == 0)
            smoke = true;
        else if (wcscmp(args[i], L"--safe-mode") == 0)
            safe = true;
        else if (wcscmp(args[i], L"--disable") == 0)
            disable = true;
    }
    LocalFree(args);
    if (disable) {
        auto w = FindWindowW(cvs::appClass, nullptr);
        if (w)
            PostMessageW(w, cvs::emergencyMessage, 0, 0);
        return 0;
    }
    HANDLE mutex =
        CreateMutexW(nullptr, FALSE, smoke ? L"Local\\CVS.Smoke.1" : L"Local\\CVS.Application.1");
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        auto w = FindWindowW(cvs::appClass, nullptr);
        if (w)
            PostMessageW(w, cvs::openMessage, 0, 0);
        CloseHandle(mutex);
        return 0;
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    HRESULT co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int result = 1;
    try {
        using VersionFn = LONG(WINAPI *)(PRTL_OSVERSIONINFOW);
        auto getVersion = reinterpret_cast<VersionFn>(
            GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
        RTL_OSVERSIONINFOW v{};
        v.dwOSVersionInfoSize = sizeof(v);
        if (!getVersion || getVersion(&v) != 0 || v.dwMajorVersion < 10 || v.dwBuildNumber < 19041)
            throw std::runtime_error("Windows 10 version 2004 (build 19041) or Windows 11 is "
                                     "required for safe capture exclusion.");
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_WIN95_CLASSES | ICC_BAR_CLASSES};
        if (!InitCommonControlsEx(&controls))
            throw std::runtime_error("Common controls initialization failed");
        cvs::App app(instance, smoke, safe);
        result = app.run(show);
    } catch (const std::exception &e) {
        if (!smoke)
            MessageBoxW(nullptr, cvs::wide(e.what()).c_str(),
                        L"Color Vision Simulator — startup failed", MB_ICONERROR | MB_OK);
        result = 1;
    }
    if (SUCCEEDED(co))
        CoUninitialize();
    if (mutex)
        CloseHandle(mutex);
    return result;
}
