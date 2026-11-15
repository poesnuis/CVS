#pragma once
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include "core/color.h"

namespace cvs {
template<class T> using ComPtr=Microsoft::WRL::ComPtr<T>;
void check_hr(HRESULT,const char* operation);
class Renderer {
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11VertexShader> vertex_;
    ComPtr<ID3D11PixelShader> pixel_;
    ComPtr<ID3D11Buffer> constants_;
    ComPtr<ID3D11SamplerState> sampler_;
    ComPtr<ID3D11RasterizerState> rasterizer_;
    struct Query { ComPtr<ID3D11Query> disjoint,start,end; bool pending{}; };
    std::array<Query,8> queries_;
    unsigned queryCursor_{};
    double gpuMs_{-1};
public:
    void initialize(ID3D11Device*,ID3D11DeviceContext*);
    void draw(ID3D11ShaderResourceView*,ID3D11RenderTargetView*,unsigned,unsigned,const Transform&,bool timing=true);
    double gpu_ms() const { return gpuMs_; }
};
}
