#include "core/color_gradient.hpp"
#include "core/config.hpp"
#include "core/globals.hpp"
#include "graphics/crosshair_drawing.hpp"
#include "graphics/overlay.hpp"

#include <algorithm>
#include <wincodec.h>
#include <wrl/client.h>

#define M_PI 3.14159265358979323846f

FLOAT Overlay::renderText(ImDrawList* list, ImFont* font, const std::string& text, const ImVec2& position,
                          const float size, const ImColor& color, const bool centerX, const bool centerY,
                          const bool outline, const bool background) {
    ImGui::PushFont(font);
    ImGui::SetWindowFontScale(size / ImGui::GetFontSize());

    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    ImVec2 renderPos = position;

    if (centerX)
        renderPos.x -= textSize.x / 2.0f;
    if (centerY)
        renderPos.y -= textSize.y / 2.0f;

    if (background) {
        const ImVec4 padding = ImVec4(2.f, .25f, 1.25f, -0.5f);
        ImVec2 bgMin = ImVec2(renderPos.x - padding.x, renderPos.y - padding.y);
        ImVec2 bgMax = ImVec2(renderPos.x + textSize.x + padding.z, renderPos.y + textSize.y + padding.w);
        list->AddRectFilled(bgMin, bgMax, ImColor(0.f, 0.f, 0.f, color.Value.w * 0.5f));
    }

    if (outline) {
        list->AddText({renderPos.x + 1.f, renderPos.y}, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
        list->AddText({renderPos.x - 1.f, renderPos.y}, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
        list->AddText({renderPos.x, renderPos.y + 1.f}, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
        list->AddText({renderPos.x, renderPos.y - 1.f}, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
    }

    list->AddText(renderPos, color, text.c_str());

    ImGui::SetWindowFontScale(1.f);
    ImGui::PopFont();

    return textSize.x;
}

void Overlay::renderCrosshair(const std::optional<crosshair::Settings>& settings, const crosshair::Frame& frame,
                              video::Size viewport) {
    if (!settings || settings->isEmpty())
        return;
    auto visible = *settings;
    if (cfg->rgbCrosshairState) {
        const auto color = colorGradient().getCurrentColor();
        visible.red = color.r;
        visible.green = color.g;
        visible.blue = color.b;
    }
    if (!viewport.valid())
        viewport = globals::getVideoSize();
    if (!viewport.valid())
        return;
    crosshair::draw(drawList_, visible, {globals::width * .5f, globals::height * .5f}, viewport.height, frame,
                    {float(globals::width) / viewport.width, float(globals::height) / viewport.height});
}

void Overlay::circularTimer(const ImVec2& pos, const float min, const float max, const float& time,
                            const int pointsCount, const float radius, const float thickness, const bool top,
                            const ImColor& color) {
    if (min >= max || pointsCount <= 0)
        return;

    ImDrawList* list = ImGui::GetWindowDrawList();

    float progress = (time - min) / (max - min);
    progress = std::clamp(progress, 0.f, 1.f);

    float startAngle = top ? 270.f : 90.f;
    float totalAngle = 360.f * progress;

    list->AddCircle(pos, radius, IM_COL32(50, 50, 50, 100), pointsCount, thickness * 0.5f);

    if (progress > 0.f) {
        float startRad = startAngle * (M_PI / 180.f);
        float endRad = (startAngle - totalAngle) * (M_PI / 180.f);

        list->PathClear();
        list->PathArcTo(pos, radius, startRad, endRad, pointsCount);
        list->PathStroke(color, 0, thickness);
    }
}

ID3D11ShaderResourceView* Overlay::loadPngTexture(unsigned char* iconPng, unsigned int iconPngSize) {
    using Microsoft::WRL::ComPtr;
    struct Apartment {
        HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        ~Apartment() {
            if (SUCCEEDED(result))
                CoUninitialize();
        }
    } apartment;
    if (FAILED(apartment.result) && apartment.result != RPC_E_CHANGED_MODE)
        return nullptr;

    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CreateStream(&stream)) || FAILED(stream->InitializeFromMemory(iconPng, iconPngSize)) ||
        FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame)) || FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0,
                                     WICBitmapPaletteTypeCustom)))
        return nullptr;

    UINT width = 0, height = 0;
    if (FAILED(frame->GetSize(&width, &height)) || !width || !height || width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
        height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        return nullptr;
    const UINT stride = width * 4, bufferSize = stride * height;
    std::vector<BYTE> pixels(bufferSize);
    if (FAILED(converter->CopyPixels(nullptr, stride, bufferSize, pixels.data())))
        return nullptr;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = pixels.data();
    data.SysMemPitch = stride;
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> view;
    if (FAILED(device_->CreateTexture2D(&desc, &data, &texture)) ||
        FAILED(device_->CreateShaderResourceView(texture.Get(), nullptr, &view)))
        return nullptr;
    return view.Detach();
}
