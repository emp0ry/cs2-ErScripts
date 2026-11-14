#pragma once
#include <filesystem>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

class Updater {
    friend struct UpdaterTestAccess;

  public:
    Updater(std::string currentVersion, std::string githubAuthor, std::string repoName, std::string exeName);
    bool checkAndUpdate();
    static std::optional<int> applyPendingUpdate();
    static void completeRestart();

  private:
    struct Release {
        std::string url, sha256;
        std::uint64_t size = 0;
    };
    std::string currentVersion, repository, exeName;
    std::string downloadJsonData() const;
    std::optional<Release> parseRelease(std::string_view data) const;
    static bool newerVersion(std::string_view candidate, std::string_view current);
    static bool verifyExecutable(const std::filesystem::path &path, std::uint64_t size, std::string_view sha256);
    static bool beginInstallation(const std::filesystem::path &candidate, const std::filesystem::path &target,
                                  std::uint64_t size, const std::string &digest);
    static bool restart(const std::filesystem::path &target, const std::filesystem::path &stage);
    static bool install(const std::filesystem::path &candidate, const std::filesystem::path &target, std::uint64_t size,
                        std::string_view sha256,
                        bool (*launch)(const std::filesystem::path &, const std::filesystem::path &) = restart);
};
