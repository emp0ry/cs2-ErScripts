#include "update/Updater.h"
#include "core/Logger.h"
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shell32.lib")

namespace {
    constexpr std::uint64_t executableLimit = 64 * 1024 * 1024;
    using FileHandle = std::unique_ptr<void, decltype(&CloseHandle)>;
    constexpr size_t metadataLimit = 1024 * 1024;
    using InternetHandle = std::unique_ptr<void, decltype(&WinHttpCloseHandle)>;

    std::optional<std::array<unsigned, 4>> versionParts(std::string_view version) {
        if (version.starts_with('v'))
            version.remove_prefix(1);
        std::array<unsigned, 4> parts{};
        for (auto &part : parts) {
            const auto dot = version.find('.');
            const auto field = version.substr(0, dot);
            const auto parsed = std::from_chars(field.data(), field.data() + field.size(), part);
            if (parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size())
                return std::nullopt;
            if (dot == version.npos)
                return parts;
            version.remove_prefix(dot + 1);
        }
        return std::nullopt;
    }
} // namespace

bool Updater::newerVersion(std::string_view candidate, std::string_view current) {
    const auto next = versionParts(candidate), previous = versionParts(current);
    return next && previous && *next > *previous;
}

Updater::Updater(std::string version, std::string author, std::string name, std::string asset)
    : currentVersion(std::move(version)), repository(author + "/" + name), exeName(std::move(asset) + ".exe") {
    const auto validName = [](std::string_view value) {
        return !value.empty() && value != "." && value != ".." &&
               std::all_of(value.begin(), value.end(), [](unsigned char c) {
                   return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' ||
                          c == '_' || c == '.';
               });
    };
    if (!validName(author) || !validName(name) || !validName(exeName) || !versionParts(currentVersion))
        throw std::invalid_argument("Invalid update repository or version");
}

std::string Updater::downloadJsonData() const {
    InternetHandle session(WinHttpOpen(L"ErScripts update check", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                       WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0),
                           WinHttpCloseHandle);
    if (!session || !WinHttpSetTimeouts(session.get(), 5000, 5000, 5000, 5000))
        return {};
    InternetHandle connection(WinHttpConnect(session.get(), L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0),
                              WinHttpCloseHandle);
    const auto path = "/repos/" + repository + "/releases/latest";
    const std::wstring widePath(path.begin(), path.end());
    InternetHandle request(connection ? WinHttpOpenRequest(connection.get(), L"GET", widePath.c_str(), nullptr,
                                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                           WINHTTP_FLAG_SECURE)
                                      : nullptr,
                           WinHttpCloseHandle);
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    constexpr auto headers = L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
    if (!request || !WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects)) ||
        !WinHttpSendRequest(request.get(), headers, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.get(), nullptr))
        return {};
    DWORD status = 0, length = sizeof(status);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &length, WINHTTP_NO_HEADER_INDEX) ||
        status != 200)
        return {};
    wchar_t contentType[128]{};
    length = sizeof(contentType);
    if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX, contentType,
                             &length, WINHTTP_NO_HEADER_INDEX))
        return {};
    const std::wstring_view type(contentType);
    if (type != L"application/json" && !type.starts_with(L"application/json;"))
        return {};
    std::string data;
    std::array<char, 4096> buffer;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    for (;;) {
        DWORD bytes = 0;
        if (std::chrono::steady_clock::now() >= deadline ||
            !WinHttpReadData(request.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &bytes) ||
            bytes > metadataLimit - data.size())
            return {};
        if (!bytes)
            return data;
        data.append(buffer.data(), bytes);
    }
}

namespace {
    bool validDigest(std::string_view value) {
        return value.size() == 64 && std::all_of(value.begin(), value.end(), [](char c) {
                   return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
               });
    }
    std::string hex(const unsigned char *data, size_t size) {
        constexpr char digits[] = "0123456789abcdef";
        std::string result;
        result.reserve(size * 2);
        for (size_t i = 0; i < size; ++i) {
            result += digits[data[i] >> 4];
            result += digits[data[i] & 15];
        }
        return result;
    }
    std::string randomID() {
        std::array<unsigned char, 16> bytes{};
        if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) <
            0)
            throw std::runtime_error("Cannot generate an update identifier");
        return hex(bytes.data(), bytes.size());
    }
    std::filesystem::path executablePath() {
        std::wstring path(32768, L'\0');
        const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (!length || length == path.size())
            throw std::runtime_error("Cannot determine the executable path");
        path.resize(length);
        return path;
    }
    FileHandle openFile(const std::filesystem::path &path) {
        const auto value = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL, nullptr);
        return FileHandle(value == INVALID_HANDLE_VALUE ? nullptr : value, CloseHandle);
    }
    bool seek(HANDLE file, LONGLONG position) {
        LARGE_INTEGER offset{};
        offset.QuadPart = position;
        return SetFilePointerEx(file, offset, nullptr, FILE_BEGIN) != FALSE;
    }
    bool read(HANDLE file, void *data, DWORD size) {
        DWORD bytes = 0;
        return ReadFile(file, data, size, &bytes, nullptr) && bytes == size;
    }
    std::optional<std::string> fileHash(HANDLE file) {
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
            return std::nullopt;
        const auto close = [](BCRYPT_ALG_HANDLE handle) { BCryptCloseAlgorithmProvider(handle, 0); };
        std::unique_ptr<void, decltype(close)> provider(algorithm, close);
        BCRYPT_HASH_HANDLE rawHash = nullptr;
        if (BCryptCreateHash(algorithm, &rawHash, nullptr, 0, nullptr, 0, 0) < 0)
            return std::nullopt;
        std::unique_ptr<void, decltype(&BCryptDestroyHash)> hash(rawHash, BCryptDestroyHash);
        if (!seek(file, 0))
            return std::nullopt;
        std::array<unsigned char, 4096> buffer;
        DWORD count = 0;
        do {
            if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr) ||
                (count && BCryptHashData(hash.get(), buffer.data(), count, 0) < 0))
                return std::nullopt;
        } while (count);
        std::array<unsigned char, 32> digest{};
        if (BCryptFinishHash(hash.get(), digest.data(), static_cast<ULONG>(digest.size()), 0) < 0)
            return std::nullopt;
        return hex(digest.data(), digest.size());
    }
    FileHandle verifiedFile(const std::filesystem::path &path, std::uint64_t expectedSize, std::string_view digest) {
        FileHandle file(nullptr, CloseHandle);
        if (!validDigest(digest) || expectedSize < sizeof(IMAGE_DOS_HEADER) || expectedSize > executableLimit)
            return file;
        file = openFile(path);
        LARGE_INTEGER size{};
        IMAGE_DOS_HEADER dos{};
        if (!file || !GetFileSizeEx(file.get(), &size) || static_cast<std::uint64_t>(size.QuadPart) != expectedSize ||
            !read(file.get(), &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
            dos.e_lfanew < static_cast<LONG>(sizeof(dos)) ||
            static_cast<std::uint64_t>(dos.e_lfanew) + sizeof(IMAGE_NT_HEADERS) > expectedSize ||
            !seek(file.get(), dos.e_lfanew))
            return FileHandle(nullptr, CloseHandle);
        IMAGE_NT_HEADERS nt{};
        if (!read(file.get(), &nt, sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE ||
            nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC ||
            !(nt.FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) ||
            (nt.FileHeader.Characteristics & IMAGE_FILE_DLL) || !nt.FileHeader.NumberOfSections ||
            nt.FileHeader.NumberOfSections > 96 || nt.FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER))
            return FileHandle(nullptr, CloseHandle);
#ifdef _WIN64
        if (nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
#else
        if (nt.FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
#endif
            return FileHandle(nullptr, CloseHandle);
        const auto actual = fileHash(file.get());
        if (!actual || *actual != digest)
            return FileHandle(nullptr, CloseHandle);
        return file;
    }
    bool copyTo(HANDLE source, const std::filesystem::path &target, bool &created) {
        const auto raw =
            CreateFileW(target.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (raw == INVALID_HANDLE_VALUE)
            return false;
        FileHandle output(raw, CloseHandle);
        created = true;
        if (!seek(source, 0))
            return false;
        std::array<char, 4096> buffer;
        DWORD count = 0;
        do {
            if (!ReadFile(source, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr))
                return false;
            DWORD written = 0;
            if (count && (!WriteFile(output.get(), buffer.data(), count, &written, nullptr) || written != count))
                return false;
        } while (count);
        return FlushFileBuffers(output.get()) != FALSE;
    }
    bool stageName(std::wstring_view name) {
        constexpr std::wstring_view prefix = L".erscripts-update-";
        if (!name.starts_with(prefix) || name.size() != prefix.size() + 32)
            return false;
        return std::all_of(name.begin() + prefix.size(), name.end(),
                           [](wchar_t c) { return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f'); });
    }
    std::optional<std::uint64_t> number(std::wstring_view text) {
        if (text.empty() || text.size() > 20)
            return std::nullopt;
        std::string ascii;
        for (const auto c : text) {
            if (c < L'0' || c > L'9')
                return std::nullopt;
            ascii += static_cast<char>(c);
        }
        std::uint64_t result = 0;
        const auto parsed = std::from_chars(ascii.data(), ascii.data() + ascii.size(), result);
        return parsed.ec == std::errc{} ? std::optional(result) : std::nullopt;
    }
    auto arguments(int &count) {
        const auto free = [](wchar_t **value) { LocalFree(value); };
        return std::unique_ptr<wchar_t *, decltype(free)>(CommandLineToArgvW(GetCommandLineW(), &count), free);
    }
    std::wstring quoted(const std::filesystem::path &path) {
        return L"\"" + path.wstring() + L"\"";
    }

    bool launchInstaller(const std::filesystem::path &candidate, const std::filesystem::path &target,
                         std::uint64_t size, const std::string &digest) {
        FileHandle parent(OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, TRUE, GetCurrentProcessId()),
                          CloseHandle);
        SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
        FileHandle ready(CreateEventW(&security, TRUE, FALSE, nullptr), CloseHandle);
        SIZE_T bytes = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
        std::vector<unsigned char> storage(bytes);
        auto *attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
        if (!parent || !ready || !InitializeProcThreadAttributeList(attributes, 1, 0, &bytes))
            return false;
        HANDLE inherited[] = {parent.get(), ready.get()};
        const bool configured = UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, inherited,
                                                          sizeof(inherited), nullptr, nullptr) != FALSE;
        STARTUPINFOEXW startup{};
        startup.StartupInfo.cb = sizeof(startup);
        startup.lpAttributeList = attributes;
        auto command = quoted(candidate) + L" --apply-update " +
                       std::to_wstring(reinterpret_cast<std::uintptr_t>(parent.get())) + L" " +
                       std::to_wstring(reinterpret_cast<std::uintptr_t>(ready.get())) + L" " + quoted(target) + L" " +
                       std::wstring(digest.begin(), digest.end()) + L" " + std::to_wstring(size);
        PROCESS_INFORMATION process{};
        const bool started = configured && CreateProcessW(candidate.c_str(), command.data(), nullptr, nullptr, TRUE,
                                                          EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW, nullptr,
                                                          nullptr, &startup.StartupInfo, &process);
        DeleteProcThreadAttributeList(attributes);
        if (!started)
            return false;
        FileHandle child(process.hProcess, CloseHandle), thread(process.hThread, CloseHandle);
        HANDLE waits[] = {ready.get(), child.get()};
        if (WaitForMultipleObjects(2, waits, FALSE, 15000) == WAIT_OBJECT_0)
            return true;
        TerminateProcess(child.get(), 1);
        WaitForSingleObject(child.get(), 5000);
        return false;
    }

    bool download(const std::string &url, const std::filesystem::path &output, std::uint64_t expected) {
        // Only the validated GitHub asset URL enters here. WinHTTP keeps certificate validation enabled.
        InternetHandle session(WinHttpOpen(L"ErScripts updater", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0),
                               WinHttpCloseHandle);
        if (!session || !WinHttpSetTimeouts(session.get(), 5000, 5000, 5000, 5000))
            return false;
        InternetHandle connection(WinHttpConnect(session.get(), L"github.com", INTERNET_DEFAULT_HTTPS_PORT, 0),
                                  WinHttpCloseHandle);
        const auto path = url.substr(std::string_view("https://github.com").size());
        const std::wstring wide(path.begin(), path.end());
        InternetHandle request(connection ? WinHttpOpenRequest(connection.get(), L"GET", wide.c_str(), nullptr,
                                                               WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                               WINHTTP_FLAG_SECURE)
                                          : nullptr,
                               WinHttpCloseHandle);
        DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
        if (!request || !WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy)) ||
            !WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
            !WinHttpReceiveResponse(request.get(), nullptr))
            return false;
        DWORD status = 0, length = sizeof(status);
        if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                 WINHTTP_HEADER_NAME_BY_INDEX, &status, &length, WINHTTP_NO_HEADER_INDEX) ||
            status != 200)
            return false;
        wchar_t type[128]{};
        length = sizeof(type);
        if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX, type, &length,
                                 WINHTTP_NO_HEADER_INDEX))
            return false;
        const std::wstring_view content(type);
        if (content != L"application/octet-stream" && content != L"application/x-msdownload" &&
            content != L"application/vnd.microsoft.portable-executable")
            return false;
        DWORD contentLength = 0;
        length = sizeof(contentLength);
        if (WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX, &contentLength, &length, WINHTTP_NO_HEADER_INDEX) &&
            contentLength != expected)
            return false;
        const auto raw =
            CreateFileW(output.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (raw == INVALID_HANDLE_VALUE)
            return false;
        FileHandle file(raw, CloseHandle);
        std::array<char, 4096> buffer;
        std::uint64_t total = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
        for (;;) {
            DWORD count = 0;
            if (std::chrono::steady_clock::now() >= deadline ||
                !WinHttpReadData(request.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &count) ||
                count > expected - total)
                return false;
            if (!count)
                return total == expected && FlushFileBuffers(file.get());
            DWORD written = 0;
            if (!WriteFile(file.get(), buffer.data(), count, &written, nullptr) || written != count)
                return false;
            total += count;
        }
    }
} // namespace

std::optional<Updater::Release> Updater::parseRelease(std::string_view data) const {
    if (data.empty() || data.size() > metadataLimit)
        return std::nullopt;
    const auto release = nlohmann::json::parse(data, nullptr, false);
    if (!release.is_object())
        return std::nullopt;
    const auto tag = release.find("tag_name"), url = release.find("html_url"), assets = release.find("assets");
    const auto draft = release.find("draft"), prerelease = release.find("prerelease"),
               published = release.find("published_at");
    if (tag == release.end() || !tag->is_string() || url == release.end() || !url->is_string() ||
        draft == release.end() || !draft->is_boolean() || draft->get<bool>() || prerelease == release.end() ||
        !prerelease->is_boolean() || prerelease->get<bool>() || published == release.end() || !published->is_string() ||
        published->get_ref<const std::string &>().empty() || assets == release.end() || !assets->is_array())
        return std::nullopt;
    const auto &version = tag->get_ref<const std::string &>();
    if (!newerVersion(version, currentVersion) ||
        *url != "https://github.com/" + repository + "/releases/tag/" + version)
        return std::nullopt;
    std::optional<Release> result;
    for (const auto &asset : *assets) {
        if (!asset.is_object() || asset.value("name", nlohmann::json{}) != exeName)
            continue;
        if (result)
            return std::nullopt;
        const auto hash = asset.find("digest"), size = asset.find("size"),
                   downloadUrl = asset.find("browser_download_url");
        if (asset.value("state", nlohmann::json{}) != "uploaded" || hash == asset.end() || !hash->is_string() ||
            size == asset.end() || !size->is_number_integer() || *size < sizeof(IMAGE_DOS_HEADER) ||
            *size > executableLimit || downloadUrl == asset.end() || !downloadUrl->is_string())
            return std::nullopt;
        const auto &digest = hash->get_ref<const std::string &>();
        const auto expected = "https://github.com/" + repository + "/releases/download/" + version + "/" + exeName;
        if (!digest.starts_with("sha256:") || !validDigest(std::string_view(digest).substr(7)) ||
            *downloadUrl != expected)
            return std::nullopt;
        result = Release{expected, digest.substr(7), size->get<std::uint64_t>()};
    }
    return result;
}

bool Updater::beginInstallation(const std::filesystem::path &candidate, const std::filesystem::path &target,
                                std::uint64_t size, const std::string &digest) {
    const auto verified = verifiedFile(candidate, size, digest);
    return verified && launchInstaller(candidate, target, size, digest);
}

bool Updater::verifyExecutable(const std::filesystem::path &path, std::uint64_t size, std::string_view hash) {
    return static_cast<bool>(verifiedFile(path, size, hash));
}

bool Updater::install(const std::filesystem::path &candidate, const std::filesystem::path &target, std::uint64_t size,
                      std::string_view digest,
                      bool (*launch)(const std::filesystem::path &, const std::filesystem::path &)) {
    const auto source = verifiedFile(candidate, size, digest);
    if (!source) {
        launch(target, {});
        return false;
    }
    const auto backup = candidate.parent_path() / L"previous.exe";
    if (!MoveFileExW(target.c_str(), backup.c_str(), MOVEFILE_WRITE_THROUGH)) {
        Logger::logWarning(std::format("Cannot preserve the previous executable: {}", GetLastError()));
        if (!launch(target, {}))
            Logger::logWarning("Previous executable could not be restarted");
        return false;
    }
    bool created = false;
    try {
        if (copyTo(source.get(), target, created)) {
            const auto installed = verifiedFile(target, size, digest);
            if (installed && launch(target, candidate.parent_path())) {
                Logger::logSuccess(
                    "Update installed and restarted. Previous executable retained in the update folder.");
                return true;
            }
        }
    } catch (const std::exception &failure) {
        Logger::logWarning(std::format("Update installation failed: {}", failure.what()));
    }
    if (created && !DeleteFileW(target.c_str()))
        Logger::logWarning(std::format("Cannot remove failed update: {}", GetLastError()));
    if (!MoveFileExW(backup.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH)) {
        const auto message =
            std::format(L"Update rollback failed ({}). Previous executable: {}", GetLastError(), backup.wstring());
        Logger::logWarning(message);
        MessageBoxW(nullptr, message.c_str(), L"ErScripts update failed", MB_OK | MB_ICONERROR);
        return false;
    }
    Logger::logWarning("Installation or restart failed; previous executable restored");
    if (!launch(target, {}))
        Logger::logWarning("Previous executable was restored but could not be restarted");
    return false;
}

bool Updater::restart(const std::filesystem::path &target, const std::filesystem::path &stage) {
    if (stage.empty()) {
        auto command = quoted(target) + L" --no-update";
        STARTUPINFOW startup{sizeof(startup)};
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(target.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr,
                            nullptr, &startup, &process))
            return false;
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return true;
    }
    const auto id = randomID();
    const std::wstring eventName = L"Local\\ErScriptsUpdate-" + std::wstring(id.begin(), id.end());
    FileHandle ready(CreateEventW(nullptr, TRUE, FALSE, eventName.c_str()), CloseHandle);
    if (!ready)
        return false;
    auto command = quoted(target) + L" --no-update --update-ready " + eventName + L" --cleanup-update " +
                   quoted(stage.filename()) + L" " + std::to_wstring(GetCurrentProcessId());
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(target.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr,
                        &startup, &process))
        return false;
    FileHandle child(process.hProcess, CloseHandle), thread(process.hThread, CloseHandle);
    HANDLE waits[] = {ready.get(), child.get()};
    if (WaitForMultipleObjects(2, waits, FALSE, 15000) == WAIT_OBJECT_0)
        return true;
    TerminateProcess(child.get(), 1);
    WaitForSingleObject(child.get(), 5000);
    return false;
}

std::optional<int> Updater::applyPendingUpdate() {
    int argc = 0;
    const auto argv = arguments(argc);
    if (!argv || argc < 2 || std::wstring_view(argv.get()[1]) != L"--apply-update")
        return std::nullopt;
    if (argc != 7)
        return 1;
    const auto parentNumber = number(argv.get()[2]), readyNumber = number(argv.get()[3]), size = number(argv.get()[6]);
    if (!parentNumber || !readyNumber || !size || !*parentNumber || !*readyNumber || *parentNumber > UINTPTR_MAX ||
        *readyNumber > UINTPTR_MAX || *parentNumber == *readyNumber)
        return 1;
    FileHandle parent(reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(*parentNumber)), CloseHandle);
    FileHandle ready(reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(*readyNumber)), CloseHandle);
    const auto candidate = executablePath();
    const std::filesystem::path target(argv.get()[4]);
    const std::wstring wideHash(argv.get()[5]);
    std::string hash;
    for (const auto c : wideHash) {
        if (c > 127)
            return 1;
        hash += static_cast<char>(c);
    }
    std::wstring parentPath(32768, L'\0');
    DWORD length = static_cast<DWORD>(parentPath.size());
    if (!GetProcessId(parent.get()) || GetProcessId(parent.get()) == GetCurrentProcessId() ||
        !QueryFullProcessImageNameW(parent.get(), 0, parentPath.data(), &length))
        return 1;
    parentPath.resize(length);
    std::error_code error;
    if (!std::filesystem::equivalent(parentPath, target, error) || error || !target.is_absolute() ||
        candidate.filename() != L"new.exe" || !stageName(candidate.parent_path().filename().wstring()) ||
        candidate.parent_path().parent_path() != target.parent_path() || !verifyExecutable(candidate, *size, hash))
        return 1;
    if (!SetEvent(ready.get()) || WaitForSingleObject(parent.get(), 30000) != WAIT_OBJECT_0)
        return 1;
    return install(candidate, target, *size, hash) ? 0 : 1;
}

void Updater::completeRestart() {
    int argc = 0;
    const auto argv = arguments(argc);
    if (!argv)
        return;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::wstring_view(argv.get()[i]) == L"--update-ready") {
            const std::wstring_view name(argv.get()[i + 1]);
            if (name.starts_with(L"Local\\ErScriptsUpdate-") && name.size() == 54) {
                FileHandle event(OpenEventW(EVENT_MODIFY_STATE, FALSE, argv.get()[i + 1]), CloseHandle);
                if (event)
                    SetEvent(event.get());
            }
        }
    }
    for (int i = 1; i + 2 < argc; ++i) {
        if (std::wstring_view(argv.get()[i]) != L"--cleanup-update" || !stageName(argv.get()[i + 1]))
            continue;
        const auto pid = number(argv.get()[i + 2]);
        if (!pid || *pid > MAXDWORD || *pid == GetCurrentProcessId())
            continue;
        FileHandle helper(OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(*pid)), CloseHandle);
        if (helper && WaitForSingleObject(helper.get(), 5000) != WAIT_OBJECT_0)
            continue;
        const auto stage = executablePath().parent_path() / argv.get()[i + 1];
        // The previous executable remains available for manual recovery after successful startup.
        DeleteFileW((stage / L"new.exe").c_str());
        RemoveDirectoryW(stage.c_str());
    }
}

bool Updater::checkAndUpdate() {
    std::filesystem::path stage;
    try {
        const auto data = downloadJsonData();
        if (data.empty()) {
            Logger::logWarning("Could not check for updates");
            return false;
        }
        const auto release = parseRelease(data);
        if (!release)
            return false;
        const auto target = executablePath();
        const auto id = randomID();
        const auto folder = target.parent_path() / (L".erscripts-update-" + std::wstring(id.begin(), id.end()));
        if (!CreateDirectoryW(folder.c_str(), nullptr)) {
            Logger::logWarning(std::format("Cannot create update directory: {}", GetLastError()));
            return false;
        }
        stage = folder;
        const auto candidate = stage / L"new.exe";
        if (download(release->url, candidate, release->size) &&
            beginInstallation(candidate, target, release->size, release->sha256)) {
            Logger::logSuccess("Update verified. Installing after shutdown...");
            return true;
        }
        Logger::logWarning(
            "Update download, SHA-256 verification or installer startup failed. Current version retained.");
    } catch (const std::exception &error) {
        Logger::logWarning(std::format("Update failed: {}", error.what()));
    }
    if (!stage.empty()) {
        if (!DeleteFileW((stage / L"new.exe").c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND)
            Logger::logWarning("Cannot remove the temporary update executable");
        if (!RemoveDirectoryW(stage.c_str()))
            Logger::logWarning("Cannot remove the temporary update directory");
    }
    return false;
}
