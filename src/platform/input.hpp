#pragma once

namespace input {
    bool setKey(int key, bool pressed) noexcept;
    using KeySender = bool (*)(int, bool) noexcept;

    class KeyPress {
        int key_;
        KeySender send_;
        bool held_;

      public:
        explicit KeyPress(int code, KeySender sender = setKey) noexcept
            : key_(code), send_(sender), held_(send_(key_, true)) {}
        ~KeyPress() {
            if (held_)
                send_(key_, false);
        }
        KeyPress(const KeyPress&) = delete;
        KeyPress& operator=(const KeyPress&) = delete;
        bool active() const noexcept { return held_; }
    };
} // namespace input
