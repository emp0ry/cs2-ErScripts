#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <future>
#include <forward_list>
#include <stop_token>

class SimpleSound {
    friend struct SimpleSoundTestAccess;

  public:
    SimpleSound(const unsigned char *data, unsigned int size);
    ~SimpleSound();
    SimpleSound(const SimpleSound &) = delete;
    SimpleSound &operator=(const SimpleSound &) = delete;

    void setVolume(int volume) noexcept;
    void play();

    bool secondBuffer(const std::string &filePath);

  private:
    float volume_ = 1.0f;
    std::wstring tempDefaultFile_, tempSecondFile_;
    std::stop_source stop_;
    std::forward_list<std::future<void>> playRequests_;

    bool loadFromBuffer(std::wstring &tempFile, const unsigned char *data, unsigned int size);
    std::wstring createTempFile(const unsigned char *data, unsigned int size);
    bool removeTempFile(std::wstring &tempFile);
    static std::wstring generateAlias();
};
