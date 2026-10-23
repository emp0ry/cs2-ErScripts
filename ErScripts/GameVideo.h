#pragma once
#include "CS2Config.h"
#include <optional>

namespace video {
    struct Size {
        int width = 0, height = 0;
        bool valid() const { return width > 0 && height > 0 && width <= 32768 && height <= 32768; }
        bool operator==(const Size &) const = default;
    };
    inline std::optional<Size> saved(std::string_view text) {
        try {
            const auto fields = cs2config::tokens(text);
            if (fields.size() < 3 || fields.front().text != "video.cfg" || !fields[1].brace || fields[1].text != "{" ||
                !fields.back().brace || fields.back().text != "}")
                return std::nullopt;
            Size size;
            for (size_t i = 2; i < fields.size() - 1; i += 2) {
                if (i + 1 >= fields.size() - 1 || fields[i].brace || fields[i + 1].brace)
                    return std::nullopt;
                int *value = fields[i].text == "setting.defaultres"         ? &size.width
                             : fields[i].text == "setting.defaultresheight" ? &size.height
                                                                            : nullptr;
                if (value) {
                    const auto &source = fields[i + 1].text;
                    const auto result = std::from_chars(source.data(), source.data() + source.size(), *value);
                    if (result.ec != std::errc{} || result.ptr != source.data() + source.size())
                        return std::nullopt;
                }
            }
            return size.valid() ? std::optional(size) : std::nullopt;
        } catch (const std::exception &) {
            return std::nullopt;
        }
    }
    class ModeChanges {
        bool reading = false;
        unsigned remaining = 0;
        Size pending;
        static std::wstring_view trim(std::wstring_view text) {
            const auto first = text.find_first_not_of(L" \t\r\n");
            if (first == text.npos)
                return {};
            return text.substr(first, text.find_last_not_of(L" \t\r\n") - first + 1);
        }
        static std::optional<int> value(std::wstring_view line, std::wstring_view name) {
            if (!line.starts_with(name))
                return std::nullopt;
            const auto arrow = line.find(L"->", name.size());
            if (arrow == line.npos)
                return std::nullopt;
            line = trim(line.substr(arrow + 2));
            if (line.empty() || line.size() > 5)
                return std::nullopt;
            int number = 0;
            for (wchar_t c : line) {
                if (c < L'0' || c > L'9')
                    return std::nullopt;
                number = number * 10 + c - L'0';
            }
            return number;
        }

      public:
        std::optional<Size> consume(std::wstring_view line) {
            constexpr std::wstring_view marker = L"[RenderSystem] Swap Chain Mode Changing:";
            if (const auto start = line.find(marker); start != line.npos && line.substr(start) == marker) {
                const auto prefix = line.substr(0, start);
                if (prefix.find_first_not_of(L"0123456789/:-. \t") == prefix.npos) {
                    reading = true;
                    remaining = 64;
                    pending = {};
                }
                return std::nullopt;
            }
            if (!reading || !remaining--) {
                reading = false;
                return std::nullopt;
            }
            line = trim(line);
            if (const auto width = value(line, L"m_nWidth:"))
                pending.width = *width;
            if (const auto height = value(line, L"m_nHeight:"))
                pending.height = *height;
            if (!pending.valid())
                return std::nullopt;
            reading = false;
            return pending;
        }
    };
} // namespace video
