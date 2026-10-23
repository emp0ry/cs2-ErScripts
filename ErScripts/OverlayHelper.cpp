#include "Overlay.h"
#include "CrosshairDrawing.h"

#define M_PI 3.14159265358979323846f

FLOAT Overlay::RenderText(ImDrawList *draw_list, ImFont *font, const std::string &text, const ImVec2 &position,
                          const float size, const ImColor &color, const bool centerX, const bool centerY,
                          const bool outline, const bool background) noexcept {
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
        draw_list->AddRectFilled(bgMin, bgMax, ImColor(0.f, 0.f, 0.f, color.Value.w * 0.5f));
    }

    if (outline) {
        draw_list->AddText({renderPos.x + 1.f, renderPos.y}, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
        draw_list->AddText({renderPos.x - 1.f, renderPos.y}, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
        draw_list->AddText({renderPos.x, renderPos.y + 1.f}, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
        draw_list->AddText({renderPos.x, renderPos.y - 1.f}, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
    }

    draw_list->AddText(renderPos, color, text.c_str());

    ImGui::SetWindowFontScale(1.f);
    ImGui::PopFont();

    return textSize.x;
}

void Overlay::RenderCrosshair(const std::optional<SteamTools::Config> &settings, const crosshair::Frame &frame,
                              video::Size viewport) {
    if (!settings || settings->isEmpty())
        return;
    auto visible = *settings;
    if (cfg->rgbCrosshairState) {
        const auto color = gradient.getCurrentColor();
        visible.red = color.r;
        visible.green = color.g;
        visible.blue = color.b;
    }
    if (!viewport.valid())
        viewport = globals::getVideoSize();
    if (!viewport.valid())
        return;
    crosshair::draw(draw_list, visible, {globals::width * .5f, globals::height * .5f}, viewport.height, frame,
                    {float(globals::width) / viewport.width, float(globals::height) / viewport.height});
}

void Overlay::CircularTimer(const ImVec2 &pos, const float min, const float max, const float &time,
                            const int points_count, const float radius, const float thickness, const bool top,
                            const ImColor &color) {
    if (min >= max || points_count <= 0)
        return;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    float progress = (time - min) / (max - min);
    progress = std::clamp(progress, 0.f, 1.f);

    float startAngle = top ? 270.f : 90.f;
    float totalAngle = 360.f * progress;

    draw_list->AddCircle(pos, radius, IM_COL32(50, 50, 50, 100), points_count, thickness * 0.5f);

    if (progress > 0.f) {
        float startRad = startAngle * (M_PI / 180.f);
        float endRad = (startAngle - totalAngle) * (M_PI / 180.f);

        draw_list->PathClear();
        draw_list->PathArcTo(pos, radius, startRad, endRad, points_count);
        draw_list->PathStroke(color, 0, thickness);
    }

    if (progress > 0.f && progress < 1.f) {
        float endAngle = startAngle - totalAngle;
        float endRad = endAngle * (M_PI / 180.f);
        ImVec2 endPoint(pos.x + radius * cos(endRad), pos.y + radius * sin(endRad));
    }
}

ID3D11ShaderResourceView *Overlay::LoadTextureFromPNGShellcode(unsigned char *icon_png, unsigned int icon_png_size) {
    HRESULT hr;
    ID3D11ShaderResourceView *textureSRV = nullptr;

    if (!g_pd3dDevice || !icon_png || icon_png_size == 0) {
        return nullptr;
    }

    CoInitialize(nullptr);

    IWICImagingFactory *wicFactory = nullptr;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
                          (void **)&wicFactory);
    if (FAILED(hr)) {
        CoUninitialize();
        return nullptr;
    }

    IWICStream *stream = nullptr;
    hr = wicFactory->CreateStream(&stream);
    if (FAILED(hr)) {
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    hr = stream->InitializeFromMemory(icon_png, icon_png_size);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to initialize stream from shellcode\n");
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    IWICBitmapDecoder *decoder = nullptr;
    hr = wicFactory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) {
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    IWICBitmapFrameDecode *frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        decoder->Release();
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    UINT width, height;
    hr = frame->GetSize(&width, &height);
    if (FAILED(hr)) {
        frame->Release();
        decoder->Release();
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    IWICFormatConverter *converter = nullptr;
    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) {
        frame->Release();
        decoder->Release();
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        converter->Release();
        frame->Release();
        decoder->Release();
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    UINT bufferSize = width * height * 4;
    std::vector<BYTE> pixelData(bufferSize);
    hr = converter->CopyPixels(nullptr, width * 4, bufferSize, pixelData.data());
    if (FAILED(hr)) {
        converter->Release();
        frame->Release();
        decoder->Release();
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixelData.data();
    initData.SysMemPitch = width * 4;

    ID3D11Texture2D *texture = nullptr;
    hr = g_pd3dDevice->CreateTexture2D(&desc, &initData, &texture);
    if (FAILED(hr)) {
        converter->Release();
        frame->Release();
        decoder->Release();
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    hr = g_pd3dDevice->CreateShaderResourceView(texture, nullptr, &textureSRV);
    texture->Release();
    converter->Release();
    frame->Release();
    decoder->Release();
    stream->Release();
    wicFactory->Release();
    CoUninitialize();

    return SUCCEEDED(hr) ? textureSRV : nullptr;
}

bool Overlay::ImageButton(const char *str_id, ImTextureID imageTexture, const ImVec2 &size) {
    ImGuiContext &g = *ImGui::GetCurrentContext();
    ImGuiWindow *window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    ImGui::PushID(str_id);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 end_pos = ImVec2(pos.x + size.x, pos.y + size.y);
    ImRect bb(pos, end_pos);

    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, ImGui::GetID(str_id))) {
        ImGui::PopID();
        return false;
    }

    bool hovered, held;
    bool clicked = ImGui::ButtonBehavior(bb, ImGui::GetID(str_id), &hovered, &held, ImGuiButtonFlags_PressedOnClick);

    ImVec4 bg_color = ImVec4(1, 1, 1, 1);
    if (hovered)
        bg_color = ImVec4(1, 1, 1, 0.5f);

    window->DrawList->PushClipRect(pos, end_pos, true);
    window->DrawList->AddImage(imageTexture, pos, end_pos, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(bg_color));
    window->DrawList->PopClipRect();

    ImGui::PopID();
    return clicked;
}
