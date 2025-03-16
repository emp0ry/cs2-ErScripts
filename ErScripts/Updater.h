#pragma once

#include <string>

class Updater {
public:
    // Sets up the updater with the current version, GitHub author, repo name, and download program name (without .exe)
    Updater(const std::string& currentVersion, const std::string& githubAuthor, const std::string& repoName, const std::string& downloadExeName);

    // Checks for updates and applies them if found
    bool checkAndUpdate();

private:
    std::string currentVersion;     // Program's current version
    std::string currentExeName;     // Name of the running executable
    std::string downloadExeName;    // Name of the program to download from GitHub (with .exe)
    std::string releasesUrl;        // GitHub releases URL
    std::string githubAuthor;       // GitHub author name
    std::string repoName;           // GitHub repository name

    // Utility functions
    std::wstring stringToWstring(const std::string& str);
    std::string getCurrentExeName(); // Get name of running exe

    // Core methods
    std::string downloadJsonData(const std::string& url);
    bool downloadFile(const std::string& url, const std::string& outputPath);
    bool replaceExecutable(const std::string& newFilePath);
    bool isValidExecutable(const std::string& filePath);
};