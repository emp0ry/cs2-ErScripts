#pragma once
#include "CrosshairGeometry.h"
#include <imgui.h>

namespace crosshair {
    struct Span {
        int x0, x1, y;
        ImU32 color;
    };
    inline ImU32 packed(Vec4 color) {
        if (color.w <= 0)
            return 0;
        const auto byte = [](float x) { return int(std::clamp(std::round(x * 255), 0.f, 255.f)); };
        return IM_COL32(byte(srgb(color.x / color.w)), byte(srgb(color.y / color.w)), byte(srgb(color.z / color.w)),
                        byte(color.w));
    }
    inline std::vector<Span> rasterize(const Scene &scene, Bounds clip) {
        std::vector<Span> result;
        std::vector<Bounds> boxes;
        int top = clip.bottom, bottom = clip.top;
        for (const auto &p : scene) {
            const auto b = bounds(p);
            boxes.push_back(b);
            top = std::min(top, b.top);
            bottom = std::max(bottom, b.bottom);
        }
        top = std::max(top, clip.top);
        bottom = std::min(bottom, clip.bottom);
        for (int y = top; y < bottom; ++y) {
            std::vector<std::pair<int, int>> ranges;
            for (const auto b : boxes)
                if (y >= b.top && y < b.bottom) {
                    const int left = std::max(b.left, clip.left), right = std::min(b.right, clip.right);
                    if (left < right)
                        ranges.emplace_back(left, right);
                }
            std::sort(ranges.begin(), ranges.end());
            for (size_t i = 0; i < ranges.size();) {
                int left = ranges[i].first, right = ranges[i].second;
                ++i;
                while (i < ranges.size() && ranges[i].first <= right) {
                    right = std::max(right, ranges[i].second);
                    ++i;
                }
                int start = left;
                ImU32 previous = 0;
                for (int x = left; x <= right; ++x) {
                    const ImU32 color = x < right ? packed(sample(scene, x, y)) : 0;
                    if (color != previous) {
                        if (previous)
                            result.push_back({start, x, y, previous});
                        start = x;
                        previous = color;
                    }
                }
            }
        }
        return result;
    }
    inline void draw(ImDrawList *list, const Settings &settings, ImVec2 center, int height, const Frame &frame,
                     ImVec2 scale = {1.f, 1.f}) {
        if (!std::isfinite(scale.x) || !std::isfinite(scale.y) || scale.x <= 0 || scale.y <= 0)
            return;
        const auto scene = build(settings, height, frame);
        center = {std::trunc(center.x), std::trunc(center.y)};
        const auto low = list->GetClipRectMin(), high = list->GetClipRectMax();
        Bounds clip{int(std::floor((low.x - center.x) / scale.x)), int(std::floor((low.y - center.y) / scale.y)),
                    int(std::ceil((high.x - center.x) / scale.x)), int(std::ceil((high.y - center.y) / scale.y))};
        struct Cache {
            ImDrawList *owner = nullptr;
            Scene scene;
            Bounds clip{};
            std::vector<Span> spans;
        };
        static std::array<Cache, 2> caches;
        Cache *cache = &caches[0];
        if (caches[1].owner == list)
            cache = &caches[1];
        else if (caches[0].owner && caches[0].owner != list)
            cache = &caches[1];
        if (cache->owner != list || cache->scene != scene || cache->clip != clip) {
            cache->owner = list;
            cache->scene = scene;
            cache->clip = clip;
            cache->spans = rasterize(scene, clip);
        }
        for (const auto &s : cache->spans)
            list->AddRectFilled({center.x + s.x0 * scale.x, center.y + s.y * scale.y},
                                {center.x + s.x1 * scale.x, center.y + (s.y + 1) * scale.y}, s.color);
    }
    inline void draw(ImDrawList *list, const Settings &settings, ImVec2 center, int height, float previewSpread = 0,
                     ImVec2 scale = {1.f, 1.f}) {
        Frame frame;
        frame.spread = previewSpread;
        frame.legacyDistance = previewSpread;
        draw(list, settings, center, height, frame, scale);
    }
} // namespace crosshair
