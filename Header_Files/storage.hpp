#pragma once
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

inline std::wstring wide(const std::string& value) {
    if (value.empty()) return {};
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (!count) throw std::runtime_error("Invalid UTF-8 text.");
    std::wstring result(count, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), count);
    return result;
}
inline fs::path executableDirectory() {
    std::wstring path(32768, L'\0');
    DWORD count = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!count || count >= path.size()) throw std::runtime_error("Cannot locate executable.");
    path.resize(count); return fs::path(path).parent_path();
}
inline fs::path findProjectRoot() {
    auto directory = executableDirectory();
    for (auto candidate = directory; !candidate.empty(); candidate = candidate.parent_path()) {
        if (fs::exists(candidate / "CMakeLists.txt") && fs::exists(candidate / "Header_Files")) return candidate;
        if (candidate == candidate.parent_path()) break;
    }
    return directory;
}
inline bool validId(const std::string& id) {
    return !id.empty() && id.size() <= 128 && id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") == std::string::npos;
}
inline std::string cleanText(std::string value) {
    for (char& c : value) if (static_cast<unsigned char>(c) < 32 || c == 127) c = ' ';
    return value;
}
inline std::string folded(std::string value) {
    for (char& c : value) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    return value;
}
struct Song {
    std::string id, title, artist;
    int duration = 0;
    json record;
    static Song parse(const json& entry) {
        if (!entry.is_object()) throw std::runtime_error("Song entry must be an object.");
        Song s;
        s.id = entry.at("id").get<std::string>(); s.title = cleanText(entry.at("title").get<std::string>());
        s.artist = cleanText(entry.value("uploader", std::string("Unknown artist")));
        if (!validId(s.id) || s.title.find_first_not_of(' ') == std::string::npos) throw std::runtime_error("Invalid song ID/title.");
        const auto& d = entry.at("duration");
        long long number = 0;
        if (d.is_string()) {
            auto text = d.get<std::string>();
            if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) throw std::runtime_error("Invalid duration.");
            number = std::stoll(text);
        } else if (d.is_number_integer()) number = d.get<long long>();
        else throw std::runtime_error("Duration must be an integer or integer string.");
        if (number < 0 || number > std::numeric_limits<int>::max()) throw std::runtime_error("Duration out of range.");
        s.duration = static_cast<int>(number); s.record = entry; return s;
    }
};
inline json readMetadata(const fs::path& file) {
    if (!fs::exists(file)) return json::array();
    std::ifstream input(file, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open info.json.");
    auto data = json::parse(input);
    if (!data.is_array()) throw std::runtime_error("info.json must contain an array.");
    return data;
}
class FileLock {
    HANDLE handle_ = INVALID_HANDLE_VALUE;
public:
    explicit FileLock(const fs::path& file) {
        handle_ = CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) throw std::runtime_error("Library busy or not writable. Close other player instances.");
    }
    ~FileLock() { if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); }
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;
};
// Antivirus/OneDrive can briefly hold a freshly closed file on Windows.
// Retry only sharing/access conflicts; persistent failures still leave the
// original library untouched and are reported to the caller.
template <typename Operation> inline bool retryFileOperation(Operation operation) {
    for (int attempt = 0; attempt < 8; ++attempt) {
        if (operation()) return true;
        const DWORD error = GetLastError();
        if (error != ERROR_SHARING_VIOLATION && error != ERROR_LOCK_VIOLATION && error != ERROR_ACCESS_DENIED) return false;
        Sleep(25);
    }
    return false;
}
inline void writeMetadata(const fs::path& file, const json& expected, const json& updated) {
    fs::create_directories(file.parent_path());
    FileLock lock(file.parent_path() / ".library.lock");
    if (readMetadata(file) != expected) throw std::runtime_error("Library changed externally. Press F5 to reload before editing.");
    auto temp = file; temp += L".new";
    try {
        std::ofstream output(temp, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Cannot create metadata temporary file.");
        output << updated.dump(4) << '\n'; output.flush();
        if (!output) throw std::runtime_error("Cannot write metadata (check disk space/permissions).");
        output.close(); if (output.fail()) throw std::runtime_error("Cannot close metadata file.");
        if (fs::exists(file)) {
            auto backup = file; backup += L".bak";
            if (!retryFileOperation([&] { return CopyFileW(file.c_str(), backup.c_str(), FALSE); })) throw std::runtime_error("Cannot back up metadata.");
        }
        if (!retryFileOperation([&] { return MoveFileExW(temp.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH); })) throw std::runtime_error("Cannot replace metadata.");
    } catch (...) { std::error_code ignored; fs::remove(temp, ignored); throw; }
}
