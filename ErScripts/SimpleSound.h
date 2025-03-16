#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <future>
#include <forward_list>

class SimpleSound {
public:
    SimpleSound(const unsigned char* data, unsigned int size); // Initalizate default sound
    ~SimpleSound(); // Delete default sound

    void setVolume(int volume) noexcept;
    void play();

    bool secondBuffer(const std::string& filePath); // Initalizate second sound
private:
    float volume_ = 1.0f;
    std::wstring tempDefaultFile_, tempSecondFile_;
    std::forward_list<std::future<void>> playRequests_;

    bool loadFromBuffer(std::wstring& tempFile, const unsigned char* data, unsigned int size);
    std::wstring createTempFile(const unsigned char* data, unsigned int size);
    bool removeTempFile(std::wstring& tempFile);
    static std::wstring generateAlias();
};