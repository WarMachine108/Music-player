#pragma once
#include <windows.h>
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include "bass.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include "maker.hpp"

class Bass {
    Maker& library;
    HSTREAM stream = 0;
    bool initialized = false, advancing = false;
    double pausedPosition = 0;
    std::string current_;
    bool fail(const std::string& action) { message = action + " (BASS error " + std::to_string(BASS_ErrorGetCode()) + ")."; return false; }
public:
    int volume = 50;
    std::string message = "Select a song and press Enter.";
    explicit Bass(Maker& songs) : library(songs) {}
    ~Bass() { stop(); if (initialized) BASS_Free(); }
    Bass(const Bass&) = delete;
    Bass& operator=(const Bass&) = delete;
    const std::string& current() const { return current_; }
    DWORD state() const { return stream ? BASS_ChannelIsActive(stream) : BASS_ACTIVE_STOPPED; }
    bool playing() const { return state() == BASS_ACTIVE_PLAYING; }
    bool paused() const { return state() == BASS_ACTIVE_PAUSED; }
    void stop() { if (stream) { BASS_StreamFree(stream); stream = 0; } advancing = false; }
    void clear() { stop(); current_.clear(); }
    bool playsong(const std::string& id) {
        if (!library.song_table.count(id)) { message = "No song selected."; return false; }
        if (!initialized) {
            if (HIWORD(BASS_GetVersion()) != BASSVERSION) { message = "BASS header/DLL mismatch."; return false; }
            if (!BASS_Init(-1, 44100, 0, nullptr, nullptr)) return fail("Audio device unavailable");
            initialized = true;
        }
        auto file = library.root / "audioloc" / (id + ".mp3");
        HSTREAM next = BASS_StreamCreateFile(FALSE, file.c_str(), 0, 0, BASS_UNICODE);
        if (!next) return fail("Cannot open audio file; check that it exists");
        if (!BASS_ChannelSetAttribute(next, BASS_ATTRIB_VOL, volume / 100.0f) || !BASS_ChannelPlay(next, FALSE)) {
            fail("Cannot play audio"); BASS_StreamFree(next); return false;
        }
        stop(); stream = next; current_ = id; advancing = true;
        message = "Playing " + library.song_table.at(id).title; return true;
    }
    void toggle(const std::string& selected) {
        if (!stream) { playsong(current_.empty() ? selected : current_); return; }
        if (playing()) {
            const double at = position();
            if (!BASS_ChannelPause(stream)) fail("Cannot pause");
            else { pausedPosition = at; message = "Paused."; }
        }
        else if (paused()) { if (!BASS_ChannelPlay(stream, FALSE)) fail("Cannot resume"); else message = "Playing."; }
        else playsong(current_);
    }
    void setvolume(int value) {
        int requested = std::clamp(value, 0, 100);
        if (stream && !BASS_ChannelSetAttribute(stream, BASS_ATTRIB_VOL, requested / 100.0f)) { fail("Cannot set volume"); return; }
        volume = requested;
    }
    void next() { auto* n = library.find(current_); if (n && n->next) playsong(n->next->data); else message = "End of playlist."; }
    void prev() { auto* n = library.find(current_); if (n && n->prev) playsong(n->prev->data); else message = "Beginning of playlist."; }
    void autochange() {
        if (!stream || !advancing || state() != BASS_ACTIVE_STOPPED) return;
        advancing = false;
        const auto* n = library.find(current_);
        for (n = n ? n->next : nullptr; n; n = n->next) if (playsong(n->data)) return;
        message = "Playlist finished. Press Enter to play another track.";
    }
    double duration() const {
        if (!stream) return 0;
        auto bytes = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
        return bytes == static_cast<QWORD>(-1) ? 0 : std::max(0.0, BASS_ChannelBytes2Seconds(stream, bytes));
    }
    double position() const {
        if (!stream) return 0;
        if (paused()) return pausedPosition;
        auto bytes = BASS_ChannelGetPosition(stream, BASS_POS_BYTE);
        return bytes == static_cast<QWORD>(-1) ? 0 : std::max(0.0, BASS_ChannelBytes2Seconds(stream, bytes));
    }
    void seek(double delta) {
        if (!stream) return;
        const double target = std::clamp(position() + delta, 0.0, duration());
        if (!BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, target), BASS_POS_BYTE)) fail("Cannot seek");
        else if (paused()) pausedPosition = target;
    }
    std::array<float, 64> spectrum() const {
        std::array<float, 64> bars{}; if (!playing()) return bars;
        std::array<float, 1024> fft{};
        if (BASS_ChannelGetData(stream, fft.data(), BASS_DATA_FFT2048) == static_cast<DWORD>(-1)) return bars;
        for (std::size_t i = 0; i < bars.size(); ++i) {
            auto lo = static_cast<std::size_t>(std::pow(1024.0, i / 64.0));
            auto hi = static_cast<std::size_t>(std::pow(1024.0, (i + 1) / 64.0));
            float peak = 0;
            for (auto bin = lo; bin < std::max(lo + 1, hi) && bin < fft.size(); ++bin) peak = std::max(peak, fft[bin]);
            bars[i] = std::clamp(std::sqrt(peak) * 3.0f, 0.0f, 1.0f);
        }
        return bars;
    }
};
