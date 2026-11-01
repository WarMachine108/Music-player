#pragma once
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>
#include "storage.hpp"

class WinHandle {
    HANDLE handle = nullptr;
public:
    explicit WinHandle(HANDLE value = nullptr) : handle(value) {}
    ~WinHandle() { if (handle && handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
    WinHandle(const WinHandle&) = delete;
    WinHandle& operator=(const WinHandle&) = delete;
    HANDLE get() const { return handle; }
};

// Windows CRT argument quoting. No cmd.exe, shell expansion or interpolation.
inline std::wstring quoteArgument(const std::wstring& argument) {
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (wchar_t c : argument) {
        if (c == L'\\') { ++slashes; continue; }
        result.append(slashes * (c == L'\"' ? 2 : 1), L'\\'); slashes = 0;
        if (c == L'\"') result += L'\\';
        result += c;
    }
    result.append(slashes * 2, L'\\'); result += L'\"'; return result;
}
struct ProcessResult { DWORD exitCode = 1; std::string output; };
inline ProcessResult runProcess(const fs::path& exe, const std::vector<std::wstring>& args,
                               const fs::path& directory, const std::atomic<bool>& cancel,
                               const std::function<void(const std::string&)>& onLine = {},
                               std::chrono::seconds timeout = std::chrono::minutes(10)) {
    std::wstring command = quoteArgument(exe.wstring());
    for (const auto& arg : args) command += L" " + quoteArgument(arg);
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE readRaw = nullptr, writeRaw = nullptr;
    if (!CreatePipe(&readRaw, &writeRaw, &security, 0)) throw std::runtime_error("Cannot create process pipe.");
    WinHandle read(readRaw), write(writeRaw);
    if (!SetHandleInformation(read.get(), HANDLE_FLAG_INHERIT, 0)) throw std::runtime_error("Cannot configure process pipe.");
    WinHandle nullInput(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, nullptr));
    if (nullInput.get() == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open subprocess input.");
    WinHandle job(CreateJobObjectW(nullptr, nullptr));
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job.get() || !SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
        throw std::runtime_error("Cannot configure download cancellation.");
    STARTUPINFOW startup{}; startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = nullInput.get(); startup.hStdOutput = write.get(); startup.hStdError = write.get();
    PROCESS_INFORMATION info{};
    if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED,
                        nullptr, directory.c_str(), &startup, &info))
        throw std::runtime_error("Cannot launch downloader (Windows error " + std::to_string(GetLastError()) + ").");
    WinHandle process(info.hProcess), thread(info.hThread);
    if (!AssignProcessToJobObject(job.get(), process.get())) {
        TerminateProcess(process.get(), 1); WaitForSingleObject(process.get(), 5000);
        throw std::runtime_error("Cannot attach download process to cancellation job.");
    }
    if (ResumeThread(thread.get()) == static_cast<DWORD>(-1)) throw std::runtime_error("Cannot start downloader.");
    ProcessResult result;
    std::string pending;
    auto start = std::chrono::steady_clock::now();
    auto drain = [&] {
        DWORD available = 0;
        while (PeekNamedPipe(read.get(), nullptr, 0, nullptr, &available, nullptr) && available) {
            char buffer[4096]; DWORD count = 0;
            if (!ReadFile(read.get(), buffer, std::min<DWORD>(available, sizeof(buffer)), &count, nullptr) || !count) break;
            result.output.append(buffer, count); pending.append(buffer, count);
            if (result.output.size() > 16 * 1024 * 1024) throw std::runtime_error("Downloader output exceeded safety limit.");
            std::size_t newline;
            while ((newline = pending.find('\n')) != std::string::npos) {
                if (onLine) onLine(cleanText(pending.substr(0, newline)));
                pending.erase(0, newline + 1);
            }
        }
    };
    while (true) {
        drain();
        if (cancel.load() || std::chrono::steady_clock::now() - start > timeout) {
            TerminateJobObject(job.get(), 1); WaitForSingleObject(process.get(), 5000);
            throw std::runtime_error(cancel.load() ? "Download canceled." : "Download timed out; try again.");
        }
        const auto state = WaitForSingleObject(process.get(), 30);
        if (state == WAIT_OBJECT_0) { drain(); break; }
        if (state == WAIT_FAILED) throw std::runtime_error("Cannot monitor downloader.");
    }
    if (!pending.empty() && onLine) onLine(cleanText(pending));
    if (!GetExitCodeProcess(process.get(), &result.exitCode)) throw std::runtime_error("Cannot read downloader exit status.");
    return result;
}
