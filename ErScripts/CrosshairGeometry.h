#pragma once
#include "CrosshairSettings.h"
#include <vector>

namespace crosshair {
    constexpr float pi = 3.14159265358979323846f;
    struct Vec4 {
        float x = 0, y = 0, z = 0, w = 0;
        bool operator==(const Vec4 &) const = default;
    };
    struct Primitive {
        Vec4 geometry;
        float angle = 0;
        int type = 0;
        float outlineStart = 0, outlineEnd = 0;
        Vec4 fill, outline;
        bool operator==(const Primitive &) const = default;
    };
    static_assert(sizeof(Primitive) == 64);
    using Scene = std::vector<Primitive>;
    struct Dimensions {
        int length, thickness, gap, splitDistance, spreadLimit;
    };
    struct Frame {
        float spread = 0, dynamicAlpha = 1, quadAngle = 45;
        bool quadCircle = false;
        float legacyDistance = 0, legacyMinimumDistance = 0;
        bool legacySplit = false;
        float shotFeedback = 0;
    };

    inline Dimensions dimensions(const Settings &s, int height) {
        const float scale = s.modern() ? float(height) / std::max(s.screenHeight, 1) : float(height) / 480.f;
        const auto positive = [&](float v) { return v > 0 ? std::max(1, int(std::round(v * scale))) : 0; };
        const auto signedSize = [&](float v) { return v < 0 ? std::min(-1, int(std::round(v * scale))) : positive(v); };
        return {positive(s.size), positive(s.thickness), s.modern() ? signedSize(s.gap) : int(4 + s.gap),
                positive(s.dynamicSplitDist), positive(float(s.dynamicSpreadLimit))};
    }
    inline float linear(float c) {
        return c <= .04045f ? c / 12.92f : std::pow((c + .055f) / 1.055f, 2.4f);
    }
    inline float srgb(float c) {
        return c <= .0031308f ? c * 12.92f : 1.055f * std::pow(c, 1.f / 2.4f) - .055f;
    }
    inline Vec4 rgba(int r, int g, int b, int a) {
        return {linear(r / 255.f), linear(g / 255.f), linear(b / 255.f), a / 255.f};
    }
    inline float smooth(float x) {
        x = std::clamp(x, 0.f, 1.f);
        return x * x * (3.f - 2.f * x);
    }

    inline Scene build(const Settings &s, int height, const Frame &frame = {}) {
        const auto d = dimensions(s, height);
        const float t = float(d.thickness), length = float(d.length), gap = float(d.gap);
        const int halfUp = (d.thickness + 1) / 2, halfDown = d.thickness - halfUp;
        const float odd = d.thickness % 2 ? .5f : 0.f;
        int r = s.red, g = s.green, b = s.blue;
        if (!s.modern() && s.color < 5) {
            constexpr int preset[][3] = {{250, 50, 50}, {50, 250, 50}, {250, 250, 50}, {50, 50, 250}, {50, 250, 250}};
            const auto i = std::clamp(s.color, 0, 4);
            r = preset[i][0];
            g = preset[i][1];
            b = preset[i][2];
        }
        Vec4 fill = rgba(r, g, b, s.modern() || s.useAlpha ? s.alpha : 255);
        Vec4 outline = rgba(s.outlineRed, s.outlineGreen, s.outlineBlue, s.drawOutline ? s.outlineAlpha : 0);
        const float before = s.drawOutline ? 1.f : 0.f, after = s.drawOutline == 1 ? 1.f : 0.f;
        Scene scene;
        scene.reserve(12);
        const auto rect = [&](float x0, float y0, float x1, float y1, Vec4 c, Vec4 o) {
            if (x1 > x0 && y1 > y0)
                scene.push_back({{x0, y0, x1 - 1, y1 - 1}, 0, 0, before, after, c, o});
        };
        const auto cross = [&](float distance, float barLength, Vec4 c, Vec4 o) {
            if (t <= 0 || barLength <= 0)
                return;
            distance = std::max(distance, 0.f);
            const float left = std::floor(-distance), right = std::ceil(distance) + (d.thickness % 2 ? -1.f : 0.f);
            const float top = std::floor(-distance - barLength),
                        bottom = std::ceil(distance) + (d.thickness % 2 ? -1.f : 0.f);
            rect(left - barLength, -float(halfUp), left, float(halfDown), c, o);
            rect(right, -float(halfUp), right + barLength, float(halfDown), c, o);
            if (!s.tStyle)
                rect(-float(halfUp), top, float(halfDown), top + barLength, c, o);
            rect(-float(halfUp), bottom, float(halfDown), bottom + barLength, c, o);
        };
        const auto circle = [&](float distance, Vec4 c, Vec4 o) {
            if (t <= 0 || distance < 0)
                return;
            scene.push_back({{-odd - .5f, -odd - .5f, distance + t, std::max(1.f, t - 1)}, 0, 1, before, after, c, o});
        };
        const auto quad = [&](float distance, float angle, Vec4 c, Vec4 o) {
            if (t <= 0)
                return;
            scene.push_back({{-odd - .5f, -odd - .5f, distance + t, t}, angle * pi / 180.f, 2, before, after, c, o});
        };
        const auto dot = [&] {
            if (t > 0)
                rect(-float(halfUp), -float(halfUp), float(halfDown), float(halfDown), fill, outline);
        };
        if (s.dot)
            dot();
        const float spread = std::trunc(std::max(std::min(frame.spread, float(d.spreadLimit) + 64), s.dot ? t : 0.f));
        auto fadedFill = fill, fadedOutline = outline;
        fadedFill.w = std::round(fill.w * 255 * frame.dynamicAlpha) / 255;
        fadedOutline.w = std::round(outline.w * 255 * frame.dynamicAlpha) / 255;
        switch (s.style) {
        case 0:
            cross(spread, length, fadedFill, fadedOutline);
            break;
        case 1:
            circle(spread, fadedFill, fadedOutline);
            break;
        case 2: {
            float distance = gap + frame.legacyDistance, innerLength = length;
            if (frame.legacySplit) {
                innerLength = std::ceil((1 - s.dynamicMaxDistSplitRatio) * length);
                auto outerFill = fill, outerOutline = outline;
                outerFill.w = std::trunc(fill.w * 255 * s.dynamicSplitAlphaOuterMod) / 255;
                outerOutline.w = std::trunc(outline.w * 255 * s.dynamicSplitAlphaOuterMod) / 255;
                cross(innerLength + distance, std::floor(length * s.dynamicMaxDistSplitRatio), outerFill, outerOutline);
                distance = std::max(0.f, gap + frame.legacyMinimumDistance);
                fill.w = std::trunc(fill.w * 255 * s.dynamicSplitAlphaInnerMod) / 255;
                outline.w = std::trunc(outline.w * 255 * s.dynamicSplitAlphaInnerMod) / 255;
            }
            cross(distance, innerLength, fill, outline);
            break;
        }
        case 3:
            circle(std::max(1.f, gap), fill, outline);
            break;
        case 4:
            cross(std::max(0.f, gap), length, fill, outline);
            break;
        case 5:
            cross(std::round(std::max(0.f, gap) + frame.shotFeedback * height / 1200.f), length, fill, outline);
            break;
        case 6:
            if (!s.dot)
                dot();
            break;
        case 7:
            if (frame.spread > 0) {
                if (frame.quadCircle) {
                    const float thick = std::max(1.f, t - 1);
                    scene.push_back({{-odd - .5f, -odd - .5f, spread + thick, thick},
                                     0,
                                     1,
                                     before,
                                     after,
                                     fadedFill,
                                     fadedOutline});
                } else
                    quad(spread, frame.quadAngle, fadedFill, fadedOutline);
            }
            cross(std::max(0.f, gap), length, fill, outline);
            break;
        case 8: {
            if (t <= 0)
                break;
            const float shift = s.dot && d.thickness % 2 ? -1.f : 0.f;
            const float left = std::floor(-std::max(0.f, gap) - t + shift), right = std::floor(std::max(0.f, gap));
            const float top = left, bottom = right;
            rect(left, top, right, top + t, fill, outline);
            rect(left, bottom, right, bottom + t, fill, outline);
            rect(left, top, left + t, bottom + t, fill, outline);
            rect(right, top, right + t, bottom + t, fill, outline);
            break;
        }
        case 9:
            quad(std::max(1.f, gap), std::clamp(s.dynamicMaxDistSplitRatio, 0.f, 1.f) * 90, fill, outline);
            break;
        }
        return scene;
    }

    inline float angleFromTop(float x, float y) {
        const float ax = std::abs(x), ay = std::abs(y), major = std::max(ax, ay);
        if (major == 0)
            return 0;
        const float ratio = std::min(ax, ay) / major, s = ratio * ratio;
        const float a = ratio * ((((.0208351f * s - .085133f) * s + .180141f) * s - .3302995f) * s + .999866f);
        float angle = ax > ay ? pi * .5f - a : a;
        if (y > 0)
            angle -= pi;
        if (std::min(-y, x) < -std::min(-y, x) && std::max(-y, x) >= -std::max(-y, x))
            angle = -angle;
        return angle;
    }
    inline std::array<float, 2> coverage(const Primitive &p, float x, float y) {
        const auto &g = p.geometry;
        if (p.type == 0)
            return {x >= g.x - p.outlineStart && y >= g.y - p.outlineStart && x <= g.z + p.outlineEnd &&
                            y <= g.w + p.outlineEnd
                        ? 1.f
                        : 0.f,
                    x >= g.x && y >= g.y && x <= g.z && y <= g.w ? 1.f : 0.f};
        x -= g.x;
        y -= g.y;
        const float radius = std::sqrt(x * x + y * y), theta = angleFromTop(x, y);
        const float blend = std::max(smooth(theta * 2 / pi), smooth(-(theta + pi * .5f) * 2 / pi));
        const float outer = p.outlineStart + (p.outlineEnd - p.outlineStart) * blend;
        const float inner = p.outlineEnd + (p.outlineStart - p.outlineEnd) * blend;
        const auto ring = [&](float a, float b) {
            return smooth(g.z + a + .5f - radius) * smooth(radius - (g.z - g.w - b - .5f));
        };
        float fill = ring(0, 0), outline = ring(outer, inner);
        if (p.type == 2) {
            const float phase = theta - std::floor(theta * 2 / pi) * (pi * .5f) - pi * .25f;
            const float inverse = 1 / std::max(radius, 1.f), edge = .5f * inverse, half = p.angle * .5f;
            fill *= smooth((half + edge - std::abs(phase)) / (2 * edge));
            outline *= smooth((half + (outer + .5f) * inverse - std::abs(phase)) * std::max(radius, 1.f));
        }
        return {outline, fill};
    }
    inline Vec4 sample(const Scene &scene, int x, int y) {
        Vec4 fill, outline;
        for (const auto &p : scene) {
            const auto mask = coverage(p, float(x), float(y));
            if (mask[1] * p.fill.w >= fill.w) {
                fill = p.fill;
                fill.w *= mask[1];
            }
            if (mask[0] * p.outline.w >= outline.w) {
                outline = p.outline;
                outline.w *= mask[0];
            }
        }
        const float a = outline.w * (1 - fill.w) * (1 - fill.w);
        return {fill.x * fill.w + outline.x * a, fill.y * fill.w + outline.y * a, fill.z * fill.w + outline.z * a,
                fill.w + a};
    }
    struct Bounds {
        int left, top, right, bottom;
        bool operator==(const Bounds &) const = default;
    };
    inline Bounds bounds(const Primitive &p) {
        const auto &g = p.geometry;
        if (p.type == 0)
            return {int(std::floor(g.x - p.outlineStart)), int(std::floor(g.y - p.outlineStart)),
                    int(std::ceil(g.z + p.outlineEnd)) + 1, int(std::ceil(g.w + p.outlineEnd)) + 1};
        const float r = g.z + std::max(p.outlineStart, p.outlineEnd) + 1;
        return {int(std::floor(g.x - r)), int(std::floor(g.y - r)), int(std::ceil(g.x + r)) + 1,
                int(std::ceil(g.y + r)) + 1};
    }
} // namespace crosshair
