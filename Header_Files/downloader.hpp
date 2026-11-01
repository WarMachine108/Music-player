#pragma once
#include <atomic>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <unordered_set>
#include "process.hpp"
#include "maker.hpp"

struct DownloadResult {
    Song song;
    fs::path staging;
};
inline std::string processError(const ProcessResult& result) {
    std::string error;
    std::istringstream input(result.output);
    for (std::string line; std::getline(input, line);) if (line.find("ERROR") != std::string::npos) error = cleanText(line);
    if (error.empty()) error = "Downloader failed (exit " + std::to_string(result.exitCode) + "). Check network and tool versions.";
    if (error.find("403") != std::string::npos) error += " Update downloader/runtime with scripts/setup-downloader.ps1.";
    return error.substr(0, 500);
}
inline DownloadResult downloadSong(const fs::path& root, const std::string& query,
                                  const std::unordered_set<std::string>& existing,
                                  const std::atomic<bool>& cancel,
                                  const std::function<void(const std::string&)>& progress) {
    if (query.find_first_not_of(" \t\r\n") == std::string::npos || query.size() > 500)
        throw std::runtime_error("Enter a song and artist (up to 500 bytes).");
    const auto tools = root / "Header_Files/ytdl";
    const auto exe = tools / "yt-dlp.exe";
    if (!fs::exists(exe) || !fs::exists(tools / "ffmpeg.exe") || !fs::exists(tools / "ffprobe.exe"))
        throw std::runtime_error("Missing yt-dlp.exe, ffmpeg.exe or ffprobe.exe in Header_Files/ytdl. See README.");
    std::vector<std::wstring> runtime;
    if (fs::exists(tools / "deno.exe")) runtime = {L"--js-runtimes", L"deno:" + (tools / "deno.exe").wstring()};
    progress("Searching YouTube...");
    auto metadataArgs = runtime;
    metadataArgs.insert(metadataArgs.end(), {L"--ignore-config", L"--encoding", L"utf-8", L"--no-playlist",
        L"--socket-timeout", L"20", L"--retries", L"2", L"--dump-json", L"--skip-download", L"--", wide("ytsearch1:" + query)});
    auto metadata = runProcess(exe, metadataArgs, root, cancel);
    if (metadata.exitCode) throw std::runtime_error(processError(metadata));
    json record;
    std::istringstream lines(metadata.output);
    for (std::string line; std::getline(lines, line);) {
        auto parsed = json::parse(line, nullptr, false);
        if (parsed.is_object() && parsed.contains("id")) { record = std::move(parsed); break; }
    }
    if (record.is_null()) throw std::runtime_error("No playable search result found.");
    // Store only the small, supported schema, with real JSON escaping.
    if (!record.contains("duration") || !record["duration"].is_number()) throw std::runtime_error("Result has no duration (live streams are unsupported).");
    double seconds = record["duration"].get<double>();
    if (!std::isfinite(seconds) || seconds < 0 || seconds > std::numeric_limits<int>::max()) throw std::runtime_error("Unsupported track duration.");
    auto song = Song::parse(json{{"id", record.at("id")}, {"title", record.at("title")},
        {"uploader", record.value("uploader", std::string("Unknown artist"))}, {"duration", static_cast<int>(seconds)}});
    if (existing.count(song.id)) throw std::runtime_error("Already in library: " + song.title);
    static std::atomic<unsigned> sequence{0};
    auto staging = root / ".downloads" / (std::to_string(GetCurrentProcessId()) + "-" + std::to_string(++sequence));
    fs::create_directories(staging);
    try {
        progress("Downloading " + song.title);
        auto args = runtime;
        args.insert(args.end(), {L"--ignore-config", L"--encoding", L"utf-8", L"--no-playlist",
            L"--socket-timeout", L"20", L"--retries", L"2", L"--newline", L"--progress", L"--progress-template",
            L"download:Downloading %(progress._percent_str)s", L"-x", L"--audio-format", L"mp3", L"--audio-quality", L"0",
            L"--ffmpeg-location", tools.wstring(), L"-o", (staging / "%(id)s.%(ext)s").wstring(),
            L"--", wide("https://www.youtube.com/watch?v=" + song.id)});
        auto result = runProcess(exe, args, root, cancel,
            [&](const std::string& line) { if (line.size() < 400) progress(line); });
        if (result.exitCode) throw std::runtime_error(processError(result));
        const auto audio = staging / (song.id + ".mp3");
        if (!fs::exists(audio) || !fs::file_size(audio)) throw std::runtime_error("Downloader produced no MP3.");
        return {std::move(song), staging};
    } catch (...) { std::error_code ignored; fs::remove_all(staging, ignored); throw; }
}

inline void commitDownload(Maker& library, const DownloadResult& result) {
    if (library.song_table.count(result.song.id)) throw std::runtime_error("Song already in library.");
    fs::create_directories(library.root / "audioloc");
    auto target = library.root / "audioloc" / (result.song.id + ".mp3");
    // A leftover file may be corrupt or owned by another process. Do not adopt
    // it silently or overwrite it; let the user recover/move it explicitly.
    if (fs::exists(target)) throw std::runtime_error("An unlisted audio file already exists for this ID. Recover its metadata or move the file before retrying.");
    fs::rename(result.staging / (result.song.id + ".mp3"), target);
    try { library.addSong(result.song); }
    catch (...) { std::error_code ignored; fs::remove(target, ignored); throw; }
}
