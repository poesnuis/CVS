#include "win/d3d_renderer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main() {
    using namespace cvs;
    try {
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> ctx;
        D3D_FEATURE_LEVEL level;
        D3D_FEATURE_LEVEL wanted[]{D3D_FEATURE_LEVEL_11_0};
        check_hr(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                                   D3D11_CREATE_DEVICE_BGRA_SUPPORT, wanted, 1, D3D11_SDK_VERSION,
                                   &device, &level, &ctx),
                 "Create WARP device");
        Renderer renderer;
        renderer.initialize(device.Get(), ctx.Get());
        constexpr unsigned width = 16, height = 16;
        std::vector<Float4> pixels(width * height);
        for (unsigned i = 0; i < pixels.size(); ++i)
            pixels[i] = {static_cast<float>((i * 73) % 256) / 255,
                         static_cast<float>((i * 127) % 256) / 255,
                         static_cast<float>((i * 199) % 256) / 255, 1};
        pixels[0] = {0, 0, 0, 1};
        pixels[1] = {1, 1, 1, 1};
        pixels[2] = {1, 0, 0, 1};
        pixels[3] = {0, 1, 0, 1};
        pixels[4] = {0, 0, 1, 1};
        D3D11_TEXTURE2D_DESC td{};
        td.Width = width;
        td.Height = height;
        td.MipLevels = td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA init{pixels.data(), width * sizeof(Float4), 0};
        ComPtr<ID3D11Texture2D> input, output, staging;
        ComPtr<ID3D11ShaderResourceView> srv;
        ComPtr<ID3D11RenderTargetView> rtv;
        check_hr(device->CreateTexture2D(&td, &init, &input), "Input texture");
        check_hr(device->CreateShaderResourceView(input.Get(), nullptr, &srv), "Input view");
        td.BindFlags = D3D11_BIND_RENDER_TARGET;
        check_hr(device->CreateTexture2D(&td, nullptr, &output), "Output texture");
        check_hr(device->CreateRenderTargetView(output.Get(), nullptr, &rtv), "Output view");
        td.BindFlags = 0;
        td.Usage = D3D11_USAGE_STAGING;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        check_hr(device->CreateTexture2D(&td, nullptr, &staging), "Staging texture");
        size_t comparisons = 0;
        float maxError = 0;
        for (int p = 0; p < 15; ++p)
            for (int m = 0; m < 3; ++m)
                for (float severity : {0.f, .45f, 1.f})
                    for (bool artistic : {false, true}) {
                        Settings s;
                        s.preset = static_cast<Preset>(p);
                        s.model = static_cast<Model>(m);
                        s.severity = severity;
                        if (artistic) {
                            s.brightness = -.3f;
                            s.contrast = 1.2f;
                            s.saturation = .7f;
                            s.cones = {.8f, .9f, 1.1f};
                            s.custom = {{.9f, .2f, -.1f, .1f, .8f, .1f, -.1f, .2f, .9f}};
                        }
                        auto t = make_transform(s);
                        renderer.draw(srv.Get(), rtv.Get(), width, height, t, false);
                        ctx->CopyResource(staging.Get(), output.Get());
                        D3D11_MAPPED_SUBRESOURCE map{};
                        check_hr(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &map),
                                 "Read test output");
                        for (unsigned y = 0; y < height; ++y) {
                            const auto *row = reinterpret_cast<const Float4 *>(
                                static_cast<const char *>(map.pData) + map.RowPitch * y);
                            for (unsigned x = 0; x < width; ++x) {
                                const auto &src = pixels[y * width + x];
                                auto expected = transform_pixel({src.x, src.y, src.z}, t);
                                const float diff = std::max({std::abs(expected.x - row[x].x),
                                                             std::abs(expected.y - row[x].y),
                                                             std::abs(expected.z - row[x].z)});
                                if (!std::isfinite(diff) || diff > 5e-5f) {
                                    ctx->Unmap(staging.Get(), 0);
                                    throw std::runtime_error("GPU/CPU mismatch at preset " +
                                                             std::to_string(p) + ", model " +
                                                             std::to_string(m) + ", pixel " +
                                                             std::to_string(y * width + x) +
                                                             ", error " + std::to_string(diff));
                                }
                                maxError = std::max(maxError, diff);
                                ++comparisons;
                            }
                        }
                        ctx->Unmap(staging.Get(), 0);
                    }
        std::cout << "PASS: D3D11 WARP shader compilation and " << comparisons
                  << " pixel comparisons; maximum channel error " << maxError << "\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
