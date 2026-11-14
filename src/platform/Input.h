#pragma once

namespace input {
    bool setKey(int key, bool pressed) noexcept;
    using KeySender = bool (*)(int, bool) noexcept;

    class KeyPress {
        int key;
        KeySender send;
        bool held;

      public:
        explicit KeyPress(int code, KeySender sender = setKey) noexcept
            : key(code), send(sender), held(send(key, true)) {}
        ~KeyPress() {
            if (held)
                send(key, false);
        }
        KeyPress(const KeyPress &) = delete;
        KeyPress &operator=(const KeyPress &) = delete;
        bool active() const noexcept { return held; }
    };
} // namespace input
