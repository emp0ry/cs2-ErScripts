#pragma once
#include <windows.h>
#include <filesystem>
#include <chrono>
#include <stdexcept>
#include <thread>

namespace fs = std::filesystem;
using namespace std::chrono_literals;

inline void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

template <typename F> void until(F predicate) {
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (!predicate()) {
        check(std::chrono::steady_clock::now() < deadline, "Operation timed out");
        std::this_thread::sleep_for(5ms);
    }
}

struct Fixture {
    fs::path original = fs::current_path();
    fs::path folder = original / ("test-data-" + std::to_string(GetCurrentProcessId()));
    Fixture() {
        check(fs::create_directory(folder), "Test directory already exists");
        fs::current_path(folder);
    }
    ~Fixture() {
        std::error_code error;
        fs::current_path(original, error);
        fs::remove_all(folder, error);
    }
};
