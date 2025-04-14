#pragma once
#include <string>
#include <vector>

class Rebuild {
public:
    Rebuild();
    bool rebuildAndRelaunch(bool should_rebuild);
    static void cleanupTempFiles();

private:
    std::string generateRandomString(size_t length);
    bool createPolymorphicBinary(std::string& output_path);
    bool replaceWithNewBinary(const std::string& newFilePath, const std::string& bat_path);
    bool isValidExecutable(const std::string& filePath);
    std::string getCurrentExeName();
    std::string getCurrentExePath();
    bool unpackIfNeeded();
    static std::string random_suffix_;
};