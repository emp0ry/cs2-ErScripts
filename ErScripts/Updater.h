// Updater.h
#pragma once
#include <string>

class Updater {
public:
    Updater(const std::string& currentVersion, const std::string& githubAuthor, const std::string& repoName, const std::string& downloadExeName);
    bool checkAndUpdate();
    static void cleanupTempFiles();

private:
    std::string currentVersion;
    std::string currentExeName;
    std::string downloadExeName;
    std::string releasesUrl;
    std::string githubAuthor;
    std::string repoName;
    static std::string random_suffix_;

    std::string generateRandomString(size_t length); // Declared
    std::string getCurrentExeName();
    std::wstring stringToWstring(const std::string& str);
    std::string downloadJsonData(const std::string& url);
    bool downloadFile(const std::string& url, const std::string& outputPath);
    bool isValidExecutable(const std::string& filePath);
    bool replaceExecutable(const std::string& newFilePath);
};