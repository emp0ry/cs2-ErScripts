#pragma once
#include <d3d11.h>

namespace graphics {
    inline HRESULT resize(ID3D11Device *device, ID3D11DeviceContext *context, IDXGISwapChain *chain,
                          ID3D11RenderTargetView *&target, UINT width, UINT height) {
        if (!device || !context || !chain || !width || !height)
            return E_INVALIDARG;
        DXGI_SWAP_CHAIN_DESC description{};
        HRESULT result = chain->GetDesc(&description);
        if (FAILED(result))
            return result;
        if (target && description.BufferDesc.Width == width && description.BufferDesc.Height == height)
            return S_FALSE;
        context->OMSetRenderTargets(0, nullptr, nullptr);
        if (target) {
            target->Release();
            target = nullptr;
        }
        const HRESULT resized = chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        ID3D11Texture2D *buffer = nullptr;
        result = chain->GetBuffer(0, IID_PPV_ARGS(&buffer));
        if (SUCCEEDED(result)) {
            result = device->CreateRenderTargetView(buffer, nullptr, &target);
            buffer->Release();
        }
        return FAILED(resized) ? resized : result;
    }
} // namespace graphics
