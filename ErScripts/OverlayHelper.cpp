#include "Overlay.h"

#define M_PI 3.14159265358979323846f

FLOAT Overlay::RenderText(ImFont* font, const std::string& text, const ImVec2& position, const float size, const ImColor& color, const bool centerX, const bool centerY, const bool outline, const bool background) noexcept {
	ImGui::PushFont(font);
	ImGui::SetWindowFontScale(size / ImGui::GetFontSize());

	// Calculate text size for centering
	ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
	ImVec2 renderPos = position;

	// Adjust position for centering
	if (centerX) renderPos.x -= textSize.x / 2.0f;
	if (centerY) renderPos.y -= textSize.y / 2.0f;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// Draw background
	if (background) {
		const ImVec4 padding = ImVec4(2.f, .25f, 1.25f, -0.5f);
		ImVec2 bgMin = ImVec2(renderPos.x - padding.x, renderPos.y - padding.y);
		ImVec2 bgMax = ImVec2(renderPos.x + textSize.x + padding.z, renderPos.y + textSize.y + padding.w);
		draw_list->AddRectFilled(bgMin, bgMax, ImColor(0.f, 0.f, 0.f, color.Value.w * 0.5f)); // Semi-transparent black background
	}

	// Draw outline
	if (outline) {
		draw_list->AddText({ renderPos.x + 1.f, renderPos.y }, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
		draw_list->AddText({ renderPos.x - 1.f, renderPos.y }, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
		draw_list->AddText({ renderPos.x, renderPos.y + 1.f }, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
		draw_list->AddText({ renderPos.x, renderPos.y - 1.f }, ImColor(0.f, 0.f, 0.f, color.Value.w), text.c_str());
	}

	// Draw main text
	draw_list->AddText(renderPos, color, text.c_str());

	ImGui::SetWindowFontScale(1.f);
	ImGui::PopFont();

	// Return the width of the rendered text
	return textSize.x;
}

#define XScale(x) (x * ((float)globals::width / 640.f))
#define YScale(y) (y * ((float)globals::height / 480.f))

void Overlay::RenderCrosshair(const std::optional<SteamTools::Config>& ch) {
    if (!ch.has_value() || ch->isEmpty()) {
        return;
    }

    ImVec2 iCenter(globals::width * 0.5f, globals::height * 0.5f);

    int r, g, b;
    switch (ch->color) {
    case 0: r = 250; g = 50;  b = 50;  break;  // Red
    case 1: r = 50;  g = 250; b = 50;  break;  // Green
    case 2: r = 250; g = 250; b = 50;  break;  // Yellow
    case 3: r = 50;  g = 50;  b = 250; break;  // Blue
    case 4: r = 50;  g = 250; b = 250; break;  // Cyan
    case 5: r = ch->red; g = ch->green; b = ch->blue; break; // Custom
    default: r = 50; g = 250; b = 50; break; // Default to green
    }
    int alpha = std::clamp(static_cast<int>(ch->alpha), 0, 255);
    if (alpha == 0) alpha = 200; // Default alpha if not specified

    RGBColor gradient_color = gradient.getCurrentColor();
    ImU32 color = cfg->rgbCrosshairState ? IM_COL32(gradient_color.r, gradient_color.g, gradient_color.b, alpha): IM_COL32(r, g, b, alpha);
    ImU32 outlineColor = IM_COL32(0, 0, 0, alpha); // Black outline

    int iBarSize = static_cast<int>(roundf(YScale(ch->size)));
    int iBarThickness = static_cast<int>(roundf(YScale(ch->thickness))) > 1 ? static_cast<int>(roundf(YScale(ch->thickness))) : 1;
    float flThick = ch->outlineThickness >= 1.f ? static_cast<int>(ch->outlineThickness) : ch->outlineThickness;
    flThick = ch->outlineThickness < 1.f && ch->outlineThickness > 0.f ? 0.5f : std::floor(ch->outlineThickness);
    int iInnerCrossDistance = 4.f + ch->gap;

    int iInnerLeft = iCenter.x - iInnerCrossDistance - iBarThickness / 2;
    int iInnerRight = iInnerLeft + 2 * iInnerCrossDistance + iBarThickness;
    int iOuterLeft = iInnerLeft - iBarSize;
    int iOuterRight = iInnerRight + iBarSize;
    int y0 = iCenter.y - iBarThickness / 2;
    int y1 = y0 + iBarThickness;

    int iInnerTop = iCenter.y - iInnerCrossDistance - iBarThickness / 2;
    int iInnerBottom = iInnerTop + 2 * iInnerCrossDistance + iBarThickness;
    int iOuterTop = iInnerTop - iBarSize;
    int iOuterBottom = iInnerBottom + iBarSize;
    int x0 = iCenter.x - iBarThickness / 2;
    int x1 = x0 + iBarThickness;

    if (ch->drawOutline && ch->outlineThickness)
        draw_list->AddRectFilled(ImVec2(iInnerRight - flThick, y0 - flThick), ImVec2(iOuterRight + flThick, y1 + flThick), outlineColor);
    draw_list->AddRectFilled(ImVec2(iInnerRight, y0), ImVec2(iOuterRight, y1), color);

    if (ch->drawOutline && ch->outlineThickness)
        draw_list->AddRectFilled(ImVec2(iOuterLeft - flThick, y0 - flThick), ImVec2(iInnerLeft + flThick, y1 + flThick), outlineColor);
    draw_list->AddRectFilled(ImVec2(iOuterLeft, y0), ImVec2(iInnerLeft, y1), color);

    if (!ch->tStyle) {
        if (ch->drawOutline && ch->outlineThickness)
            draw_list->AddRectFilled(ImVec2(x0 - flThick, iOuterTop - flThick), ImVec2(x1 + flThick, iInnerTop + flThick), outlineColor);
        draw_list->AddRectFilled(ImVec2(x0, iOuterTop), ImVec2(x1, iInnerTop), color);
    }

    if (ch->drawOutline && ch->outlineThickness)
        draw_list->AddRectFilled(ImVec2(x0 - flThick, iInnerBottom - flThick), ImVec2(x1 + flThick, iOuterBottom + flThick), outlineColor);
    draw_list->AddRectFilled(ImVec2(x0, iInnerBottom), ImVec2(x1, iOuterBottom), color);

    if (ch->dot) {
        int x0 = iCenter.x - iBarThickness / 2;
        int x1 = x0 + iBarThickness;
        int y0 = iCenter.y - iBarThickness / 2;
        int y1 = y0 + iBarThickness;

        if (ch->drawOutline && ch->outlineThickness)
            draw_list->AddRectFilled(ImVec2(x0 - flThick, y0 - flThick), ImVec2(x1 + flThick, y1 + flThick), outlineColor);
        draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color);
    }
}

void Overlay::CircularTimer(const ImVec2& pos, const float min, const float max, const float& time,
    const int points_count, const float radius, const float thickness,
    const bool top, const ImColor& color) {
    if (min >= max || points_count <= 0) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Calculate progress (0.0 to 1.0)
    float progress = (time - min) / (max - min);
    progress = std::clamp(progress, 0.f, 1.f);

    // Starting angle (270° = top, 90° = bottom)
    float startAngle = top ? 270.f : 90.f;
    float totalAngle = 360.f * progress;

    // Draw background circle
    draw_list->AddCircle(pos, radius, IM_COL32(50, 50, 50, 100), points_count, thickness * 0.5f);

    // Draw the progress arc as a continuous curve
    if (progress > 0.f) {
        float startRad = startAngle * (M_PI / 180.f);
        float endRad = (startAngle - totalAngle) * (M_PI / 180.f);

        draw_list->PathClear();
        draw_list->PathArcTo(pos, radius, startRad, endRad, points_count);
        draw_list->PathStroke(color, 0, thickness);
    }

    // Draw moving endpoint circle
    if (progress > 0.f && progress < 1.f) {
        float endAngle = startAngle - totalAngle;
        float endRad = endAngle * (M_PI / 180.f);
        ImVec2 endPoint(pos.x + radius * cos(endRad), pos.y + radius * sin(endRad));
        //draw_list->AddCircleFilled(endPoint, thickness * 1.5f, color);
    }
}

ID3D11ShaderResourceView* Overlay::LoadTextureFromPNGShellcode(unsigned char* icon_png, unsigned int icon_png_size) {
    HRESULT hr;
    ID3D11ShaderResourceView* textureSRV = nullptr;

    if (!g_pd3dDevice || !icon_png || icon_png_size == 0) {
        return nullptr;
    }

    CoInitialize(nullptr);

    IWICImagingFactory* wicFactory = nullptr;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_IWICImagingFactory, (void**)&wicFactory);
    if (FAILED(hr)) {
        CoUninitialize();
        return nullptr;
    }

    IWICStream* stream = nullptr;
    hr = wicFactory->CreateStream(&stream);
    if (FAILED(hr)) {
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    // Use the passed icon_png and icon_png_size
    hr = stream->InitializeFromMemory(icon_png, icon_png_size);
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to initialize stream from shellcode\n");
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    IWICBitmapDecoder* decoder = nullptr;
    hr = wicFactory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) {
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    IWICBitmapFrameDecode* frame = nullptr;
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

    IWICFormatConverter* converter = nullptr;
    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) {
        frame->Release();
        decoder->Release();
        stream->Release();
        wicFactory->Release();
        CoUninitialize();
        return nullptr;
    }

    hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
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

    ID3D11Texture2D* texture = nullptr;
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

bool Overlay::ImageButton(const char* str_id, ImTextureID imageTexture, const ImVec2& size) {
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* window = g.CurrentWindow;
    if (window->SkipItems) return false;

    // Push style variables for button appearance
    ImGui::PushID(str_id);

    // Get positions and calculate bounds
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 end_pos = ImVec2(pos.x + size.x, pos.y + size.y);
    ImRect bb(pos, end_pos);

    // Update item rect for interaction
    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, ImGui::GetID(str_id))) {
        ImGui::PopID();
        return false;
    }

    // Check interaction states
    bool hovered, held;
    bool clicked = ImGui::ButtonBehavior(bb, ImGui::GetID(str_id), &hovered, &held, ImGuiButtonFlags_PressedOnClick);

    // Draw rounded background for hover/active states
    ImVec4 bg_color = ImVec4(1, 1, 1, 1); // Transparent by default
    if (hovered) bg_color = ImVec4(1, 1, 1, 0.5f); // Subtle dark background on hover

    // Draw the image with rounded corners and adjusted tint color
    window->DrawList->PushClipRect(pos, end_pos, true);
    window->DrawList->AddImage(imageTexture, pos, end_pos, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(bg_color));
    window->DrawList->PopClipRect();

    ImGui::PopID();
    return clicked;
}