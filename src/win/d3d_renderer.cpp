#include "d3d_renderer.h"
#include "shader_source.h"
#include <cstdio>
#include <d3dcompiler.h>
#include <stdexcept>
#include <string>

namespace cvs {
void check_hr(HRESULT hr, const char *operation) {
    if (FAILED(hr)) {
        char code[24];
        std::snprintf(code, sizeof(code), " (0x%08lX)", static_cast<unsigned long>(hr));
        throw std::runtime_error(std::string(operation) + code);
    }
}
void Renderer::initialize(ID3D11Device *d, ID3D11DeviceContext *c) {
    device_ = d;
    context_ = c;
    auto compile = [](const char *entry, const char *target) {
        ComPtr<ID3DBlob> shader, error;
        auto hr = D3DCompile(shaderSource, sizeof(shaderSource) - 1, "embedded simulation.hlsl",
                             nullptr, nullptr, entry, target,
                             D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
                             &shader, &error);
        if (FAILED(hr))
            throw std::runtime_error(
                error ? std::string(static_cast<const char *>(error->GetBufferPointer()),
                                    error->GetBufferSize())
                      : "Shader compilation failed");
        return shader;
    };
    auto vs = compile("VS", "vs_5_0"), ps = compile("PS", "ps_5_0");
    check_hr(d->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &vertex_),
             "Create vertex shader");
    check_hr(d->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pixel_),
             "Create pixel shader");
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = sizeof(Transform);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    check_hr(d->CreateBuffer(&bd, nullptr, &constants_), "Create shader constants");
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    check_hr(d->CreateSamplerState(&sd, &sampler_), "Create sampler");
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    check_hr(d->CreateRasterizerState(&rd, &rasterizer_), "Create rasterizer");
    for (auto &q : queries_) {
        D3D11_QUERY_DESC desc{D3D11_QUERY_TIMESTAMP_DISJOINT, 0};
        check_hr(d->CreateQuery(&desc, &q.disjoint), "Create timing query");
        desc.Query = D3D11_QUERY_TIMESTAMP;
        check_hr(d->CreateQuery(&desc, &q.start), "Create timestamp");
        check_hr(d->CreateQuery(&desc, &q.end), "Create timestamp");
    }
}
void Renderer::draw(ID3D11ShaderResourceView *source, ID3D11RenderTargetView *target, unsigned w,
                    unsigned h, const Transform &t, bool timing) {
    for (auto &q : queries_)
        if (q.pending) {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
            UINT64 start{}, end{};
            if (context_->GetData(q.disjoint.Get(), &disjoint, sizeof(disjoint),
                                  D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                context_->GetData(q.start.Get(), &start, sizeof(start),
                                  D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                context_->GetData(q.end.Get(), &end, sizeof(end), D3D11_ASYNC_GETDATA_DONOTFLUSH) ==
                    S_OK) {
                if (!disjoint.Disjoint && disjoint.Frequency)
                    gpuMs_ = 1000.0 * static_cast<double>(end - start) /
                             static_cast<double>(disjoint.Frequency);
                q.pending = false;
            }
        }
    auto &q = queries_[queryCursor_++ % queries_.size()];
    bool measure = timing && !q.pending;
    if (measure) {
        context_->Begin(q.disjoint.Get());
        context_->End(q.start.Get());
    }
    context_->UpdateSubresource(constants_.Get(), 0, nullptr, &t, 0, 0);
    D3D11_VIEWPORT vp{0, 0, static_cast<float>(w), static_cast<float>(h), 0, 1};
    context_->RSSetViewports(1, &vp);
    context_->RSSetState(rasterizer_.Get());
    context_->OMSetRenderTargets(1, &target, nullptr);
    context_->IASetInputLayout(nullptr);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vertex_.Get(), nullptr, 0);
    context_->PSSetShader(pixel_.Get(), nullptr, 0);
    auto *cb = constants_.Get();
    auto *sampler = sampler_.Get();
    context_->PSSetConstantBuffers(0, 1, &cb);
    context_->PSSetSamplers(0, 1, &sampler);
    context_->PSSetShaderResources(0, 1, &source);
    context_->Draw(3, 0);
    ID3D11ShaderResourceView *empty = nullptr;
    context_->PSSetShaderResources(0, 1, &empty);
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    if (measure) {
        context_->End(q.end.Get());
        context_->End(q.disjoint.Get());
        q.pending = true;
    }
}
} // namespace cvs
