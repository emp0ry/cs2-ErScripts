#include "update/Updater.h"
#include "game/CS2Config.h"
#include "TestSupport.h"
#include <nlohmann/json.hpp>
#include <wincrypt.h>
#include <shellapi.h>
#include <iostream>

struct UpdaterTestAccess {
    static auto release(const Updater &updater, std::string_view data) { return updater.parseRelease(data); }
    static bool verify(const fs::path &path, std::uint64_t size, std::string_view digest) {
        return Updater::verifyExecutable(path, size, digest);
    }
    static bool install(const fs::path &candidate, const fs::path &target, std::uint64_t size, std::string_view digest,
                        bool (*launch)(const fs::path &, const fs::path &)) {
        return Updater::install(candidate, target, size, digest, launch);
    }
    static bool begin(const fs::path &candidate, const fs::path &target, std::uint64_t size,
                      const std::string &digest) {
        return Updater::beginInstallation(candidate, target, size, digest);
    }
    static bool newer(std::string_view candidate, std::string_view current) {
        return Updater::newerVersion(candidate, current);
    }
};

fs::path testExecutable() {
    std::wstring path(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    check(length > 0 && length < path.size(), "Cannot locate test executable");
    path.resize(length);
    return path;
}

std::string referenceHash(const fs::path &path) {
    // CryptoAPI provides an independent reference for the updater's BCrypt implementation.
    HCRYPTPROV provider = 0;
    check(CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT),
          "Reference provider failed");
    HCRYPTHASH hash = 0;
    check(CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash), "Reference hash failed");
    std::ifstream input(path, std::ios::binary);
    check(static_cast<bool>(input), "Reference file failed");
    std::array<char, 16384> data;
    while (input.read(data.data(), data.size()) || input.gcount())
        check(CryptHashData(hash, reinterpret_cast<const BYTE *>(data.data()), static_cast<DWORD>(input.gcount()), 0),
              "Reference hashing failed");
    std::array<BYTE, 32> digest;
    DWORD length = static_cast<DWORD>(digest.size());
    check(CryptGetHashParam(hash, HP_HASHVAL, digest.data(), &length, 0), "Reference digest failed");
    CryptDestroyHash(hash);
    CryptReleaseContext(provider, 0);
    std::string result;
    for (const auto byte : digest) {
        result += "0123456789abcdef"[byte >> 4];
        result += "0123456789abcdef"[byte & 15];
    }
    return result;
}

void updaterTests() {
    Updater updater("2.0.1", "emp0ry", "cs2-ErScripts", "ErScripts");
    nlohmann::json release = {
        {"tag_name", "v2.0.2"},
        {"html_url", "https://github.com/emp0ry/cs2-ErScripts/releases/tag/v2.0.2"},
        {"draft", false},
        {"prerelease", false},
        {"published_at", "2026-11-14T00:00:00Z"},
        {"assets", nlohmann::json::array(
                       {{{"name", "ErScripts.exe"},
                         {"state", "uploaded"},
                         {"size", 1024},
                         {"digest", "sha256:" + std::string(64, 'a')},
                         {"browser_download_url",
                          "https://github.com/emp0ry/cs2-ErScripts/releases/download/v2.0.2/ErScripts.exe"}}})}};
    check(UpdaterTestAccess::release(updater, release.dump()).has_value(), "Valid release rejected");
    const auto reject = [&](auto change, const char *message) {
        auto invalid = release;
        change(invalid);
        check(!UpdaterTestAccess::release(updater, invalid.dump()), message);
    };
    reject([](auto &v) { v["assets"][0].erase("digest"); }, "Missing digest accepted");
    reject([](auto &v) { v["assets"][0]["digest"] = "sha256:bad"; }, "Invalid digest accepted");
    reject([](auto &v) { v["assets"][0]["digest"] = "sha256:" + std::string(64, 'z'); }, "Nonhex digest accepted");
    reject([](auto &v) { v["assets"][0]["size"] = 65 * 1024 * 1024; }, "Oversized asset accepted");
    reject([](auto &v) { v["assets"][0]["size"] = -1; }, "Negative asset size accepted");
    reject([](auto &v) { v["assets"][0]["browser_download_url"] = "https://evil.test/ErScripts.exe"; },
           "Foreign download accepted");
    reject([](auto &v) { v["draft"] = true; }, "Draft release accepted");
    reject([](auto &v) { v["prerelease"] = true; }, "Prerelease accepted");
    reject([](auto &v) { v["assets"].push_back(v["assets"][0]); }, "Duplicate executable accepted");
    check(!UpdaterTestAccess::release(updater, "{bad") && !UpdaterTestAccess::release(updater, "[]"),
          "Malformed release accepted");

    const auto root = fs::absolute(fs::path(L"update % & ! '$ \u042F"));
    fs::create_directory(root);
    const auto stage = root / ".erscripts-update-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    fs::create_directory(stage);
    const auto candidate = stage / "new.exe", target = root / "original.exe", backup = stage / "previous.exe";
    fs::copy_file(testExecutable(), candidate);
    const auto size = fs::file_size(candidate);
    const auto digest = referenceHash(candidate);
    check(UpdaterTestAccess::verify(candidate, size, digest), "BCrypt digest differs from the reference");
    check(!UpdaterTestAccess::verify(candidate, size - 1, digest), "Incomplete executable accepted");
    check(!UpdaterTestAccess::verify(candidate, size, std::string(64, '0')), "Hash mismatch accepted");
    check(!UpdaterTestAccess::begin(candidate, target, size, std::string(64, '0')), "Unverified executable launched");
    {
        std::fstream file(candidate, std::ios::binary | std::ios::in | std::ios::out);
        file.seekg(-1, std::ios::end);
        char value = 0;
        file.read(&value, 1);
        value ^= 1;
        file.seekp(-1, std::ios::end);
        file.write(&value, 1);
    }
    check(!UpdaterTestAccess::verify(candidate, size, digest), "Corrupted executable accepted");
    fs::copy_file(testExecutable(), candidate, fs::copy_options::overwrite_existing);
    cs2config::write(target, "previous executable");
    const auto succeeds = +[](const fs::path &, const fs::path &) { return true; };
    check(!UpdaterTestAccess::install(candidate, target, size, std::string(64, '0'), succeeds) &&
              cs2config::read(target) == "previous executable" && !fs::exists(backup),
          "Unverified update touched the original");
    HANDLE locked = CreateFileW(target.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    check(locked != INVALID_HANDLE_VALUE, "Cannot lock fixture");
    const bool blocked = UpdaterTestAccess::install(candidate, target, size, digest, succeeds);
    CloseHandle(locked);
    check(!blocked && cs2config::read(target) == "previous executable", "Locked original was damaged");
    static int attempts = 0;
    attempts = 0;
    const auto failRestart = +[](const fs::path &file, const fs::path &folder) {
        ++attempts;
        return folder.empty() && cs2config::read(file) == "previous executable";
    };
    check(!UpdaterTestAccess::install(candidate, target, size, digest, failRestart) && attempts == 2 &&
              cs2config::read(target) == "previous executable" && !fs::exists(backup),
          "Restart failure did not roll back");
    check(UpdaterTestAccess::install(candidate, target, size, digest, succeeds) &&
              cs2config::read(backup) == "previous executable" && UpdaterTestAccess::verify(target, size, digest),
          "Installation failed");
    fs::remove(backup);
    fs::remove(target);
    fs::copy_file(testExecutable(), target);
    auto command = L"\"" + target.wstring() + L"\" --updater-parent \"" + candidate.wstring() + L"\" " +
                   std::wstring(digest.begin(), digest.end());
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    check(CreateProcessW(target.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                         root.c_str(), &startup, &process),
          "Cannot start isolated updater parent");
    const auto wait = WaitForSingleObject(process.hProcess, 30000);
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    check(wait == WAIT_OBJECT_0 && code == 0, "Verified installer handoff failed");
    until([&] { return fs::exists(root / "restarted.txt"); });
    check(UpdaterTestAccess::verify(target, size, digest) && UpdaterTestAccess::verify(backup, size, digest) &&
              !fs::exists(candidate),
          "Automatic install, restart or cleanup failed");
    fs::remove(root / "restarted.txt");
    fs::remove(backup);
    fs::copy_file(testExecutable(), candidate);
    {
        std::ofstream old(target, std::ios::binary | std::ios::app);
        old << "previous version";
    }
    const auto originalSize = fs::file_size(target);
    const auto originalHash = referenceHash(target);
    command = L"\"" + target.wstring() + L"\" --updater-parent-fail \"" + candidate.wstring() + L"\" " +
              std::wstring(digest.begin(), digest.end());
    process = {};
    check(CreateProcessW(target.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                         root.c_str(), &startup, &process),
          "Cannot start rollback fixture parent");
    const auto failedWait = WaitForSingleObject(process.hProcess, 30000);
    code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    check(failedWait == WAIT_OBJECT_0 && code == 0, "Rollback fixture handoff failed");
    until([&] { return fs::exists(root / "restarted.txt"); });
    check(UpdaterTestAccess::verify(target, originalSize, originalHash) && !fs::exists(backup),
          "Real failed startup did not restore and restart the previous executable");
}

int main(int argc, char *argv[]) {
    if (GetEnvironmentVariableW(L"ERSCRIPTS_TEST_FAIL_RESTART", nullptr, 0) && argc > 3 &&
        std::string_view(argv[1]) == "--no-update")
        return 73;
    if (const auto result = Updater::applyPendingUpdate())
        return *result;
    Updater::completeRestart();
    if (argc > 1 && std::string_view(argv[1]) == "--no-update") {
        cs2config::write("restarted.txt", "ready");
        return 0;
    }
    if (argc == 4 &&
        (std::string_view(argv[1]) == "--updater-parent" || std::string_view(argv[1]) == "--updater-parent-fail")) {
        if (std::string_view(argv[1]) == "--updater-parent-fail")
            SetEnvironmentVariableW(L"ERSCRIPTS_TEST_FAIL_RESTART", L"1");
        int count = 0;
        auto **wide = CommandLineToArgvW(GetCommandLineW(), &count);
        const fs::path candidate(wide[2]);
        LocalFree(wide);
        return UpdaterTestAccess::begin(candidate, testExecutable(), fs::file_size(candidate), argv[3]) ? 0 : 1;
    }
    try {
        Fixture fixture;
        updaterTests();
        std::cout << "PASS: updater verification, rollback and process handoff\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
