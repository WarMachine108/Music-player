#pragma once
#include <chrono>
#include <deque>
#include <future>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>
#include "bassplayer.hpp"
#include "downloader.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/linear_gradient.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>

inline std::string formatTime(double value) {
    auto seconds = std::max(0LL, static_cast<long long>(value));
    std::ostringstream out; out << seconds / 60 << ':' << std::setfill('0') << std::setw(2) << seconds % 60;
    return out.str();
}

namespace theme {
inline ftxui::Color bg() { return ftxui::Color::RGB(12, 15, 24); }
inline ftxui::Color panel() { return ftxui::Color::RGB(19, 24, 36); }
inline ftxui::Color line() { return ftxui::Color::RGB(43, 51, 70); }
inline ftxui::Color text() { return ftxui::Color::RGB(226, 232, 243); }
inline ftxui::Color muted() { return ftxui::Color::RGB(140, 154, 179); }
inline ftxui::Color mint() { return ftxui::Color::RGB(111, 234, 205); }
inline ftxui::Color violet() { return ftxui::Color::RGB(174, 148, 250); }
inline ftxui::Color gold() { return ftxui::Color::RGB(241, 196, 124); }
inline ftxui::Color red() { return ftxui::Color::RGB(255, 146, 157); }
}

class PlayerUI {
    enum class Dialog { None, Download, Search, Delete, Help };
    Maker& library;
    Bass& player;
    std::string sort = "Title", filter, inputText, deleteId;
    bool ascending = true, error = false;
    int selected = 0;
    Dialog dialog = Dialog::None;
    std::vector<std::string> visible;
    std::deque<std::string> log;
    ftxui::Component input;
    std::future<DownloadResult> download;
    std::atomic<bool> cancel{false};
    std::mutex progressMutex;
    std::string progress;
    std::array<float, 64> levels{};
    std::string lastPlaying;
    std::vector<ftxui::Box> rowBoxes;
    std::string selectedId() const { return visible.empty() ? "" : visible.at(selected); }
    bool busy() const { return download.valid(); }
    void notify(const std::string& message, bool failed = false) {
        player.message = cleanText(message); error = failed;
        log.push_back(player.message); if (log.size() > 7) log.pop_front();
    }
    void refresh(const std::string& keep = "") {
        visible.clear();
        for (const auto& id : library.orderedIds()) {
            const auto& s = library.song_table.at(id);
            if (filter.empty() || folded(s.title + " " + s.artist).find(folded(filter)) != std::string::npos) visible.push_back(id);
        }
        selected = visible.empty() ? 0 : std::clamp(selected, 0, static_cast<int>(visible.size()) - 1);
        auto found = std::find(visible.begin(), visible.end(), keep);
        if (found != visible.end()) selected = static_cast<int>(found - visible.begin());
    }
    void open(Dialog value) {
        dialog = value; inputText = value == Dialog::Search ? filter : "";
        input = ftxui::Input(&inputText, value == Dialog::Search ? "Filter by title or artist" : "Song title + artist");
        input->TakeFocus();
    }
    void startDownload() {
        if (busy()) { notify("A download is already running."); return; }
        if (library.readOnly) { notify("Repair invalid metadata before downloading.", true); return; }
        if (inputText.find_first_not_of(" \t\r\n") == std::string::npos || inputText.size() > 500) {
            notify("Enter a song and artist (up to 500 bytes).", true); return;
        }
        std::unordered_set<std::string> existing;
        for (const auto& entry : library.song_table) existing.insert(entry.first);
        cancel.store(false);
        { std::lock_guard<std::mutex> guard(progressMutex); progress = "Starting download..."; }
        auto query = inputText; auto root = library.root;
        download = std::async(std::launch::async, [this, root, query, existing] {
            return downloadSong(root, query, existing, cancel, [this](const std::string& value) {
                std::lock_guard<std::mutex> guard(progressMutex); progress = value;
            });
        });
        dialog = Dialog::None; notify("Download started. You can keep listening; Esc cancels the download.");
    }
    void removeSelected() {
        const bool current = player.current() == deleteId;
        const bool wasPlaying = player.playing();
        std::string next;
        if (const auto* node = library.find(deleteId)) {
            if (node->next) next = node->next->data;
            else if (node->prev) next = node->prev->data;
        }
        if (current) player.stop(); // Release Windows file handle before deletion.
        try {
            auto warning = library.deleteSong(deleteId);
            if (current) { player.clear(); if (wasPlaying && !next.empty()) player.playsong(next); }
            refresh(next); notify(warning.empty() ? "Song removed from your library." : warning, !warning.empty());
        } catch (...) {
            if (current && wasPlaying) player.playsong(deleteId);
            throw;
        }
    }
    ftxui::Element divider() const { return ftxui::separator() | ftxui::color(theme::line()); }
    ftxui::Element chip(const std::string& label, bool active) const {
        using namespace ftxui;
        return text(" " + label + " ") | color(active ? theme::bg() : theme::muted()) |
            bgcolor(active ? theme::mint() : theme::panel()) | bold;
    }
    ftxui::Element recordArt() const {
        using namespace ftxui;
        Canvas art(60, 40);
        for (int r = 18; r > 4; r -= 2)
            art.DrawPointCircle(30, 20, r, Color::Interpolate((18 - r) / 14.0f, theme::violet(), theme::mint()));
        art.DrawPointCircleFilled(30, 20, 4, theme::mint());
        art.DrawPointCircleFilled(30, 20, 1, theme::bg());
        return canvas(std::move(art)) | center;
    }
    ftxui::Element spectrum(int width) const {
        using namespace ftxui;
        int bars = std::clamp(width / 2, 8, 32);
        Elements rows;
        for (int row = 3; row >= 0; --row) {
            Elements cells;
            for (int i = 0; i < bars; ++i) {
                float level = levels[static_cast<std::size_t>(i * 64 / bars)];
                auto tint = Color::Interpolate(i / static_cast<float>(bars - 1), theme::violet(), theme::mint());
                cells.push_back(text(level * 4 > row ? "▌" : "·") | color(level * 4 > row ? tint : theme::line()) | flex);
            }
            rows.push_back(hbox(std::move(cells)));
        }
        return vbox(std::move(rows));
    }
    ftxui::Element modal(int width, int height) {
        using namespace ftxui;
        std::string title;
        Elements content{text("")};
        if (dialog == Dialog::Download || dialog == Dialog::Search) {
            title = dialog == Dialog::Download ? " ADD TO YOUR COLLECTION " : " FIND YOUR MUSIC ";
            content.push_back(text(dialog == Dialog::Download ? "Search YouTube by song title and artist." : "Filter the library by title or artist.") | color(theme::muted()));
            content.push_back(text("")); content.push_back(input->Render() | border | color(theme::mint()));
            content.push_back(text(""));
            content.push_back(text(dialog == Dialog::Download ? "Enter Download   Esc Cancel" : "Enter Apply   Esc Cancel") | color(theme::mint()));
            content.push_back(paragraph(player.message) | color(error ? theme::red() : theme::muted()));
        } else if (dialog == Dialog::Delete) {
            title = " REMOVE SONG? ";
            content.push_back(paragraph(library.song_table.at(deleteId).title) | bold);
            content.push_back(text(""));
            content.push_back(paragraph("This removes the song from your library and deletes its local audio and artwork.") | color(theme::muted()));
            content.push_back(text("")); content.push_back(text("Y Remove   Esc / N Cancel") | color(theme::red()));
        } else {
            title = " MAKE YOURSELF AT HOME ";
            for (const auto* line : {"Up / Down    Browse        Enter       Play selected", "Space / S    Pause         N / P       Next / Previous",
                "D / A, +/-   Volume        Left/Right  Seek 5 seconds", "1 / 2 / 3    Sort          X           Reverse order",
                "/            Filter        Esc         Clear filter / Cancel", "F2           Download      R / Delete  Remove selected",
                "F5           Reload        Q           Quit", "", "Recent = order added. Missing files are marked in the library.",
                "Downloads select the first YouTube result. Esc cancels.", "", "Recent activity:"}) content.push_back(text(line) | color(theme::muted()));
            if (height >= 32) {
                const auto start = log.size() > 4 ? log.size() - 4 : 0;
                for (std::size_t i = start; i < log.size(); ++i)
                    content.push_back(paragraph(log[i]) | size(HEIGHT, LESS_THAN, 2));
            } else if (!log.empty()) content.push_back(text(log.back()));
            content.push_back(text("")); content.push_back(text("Esc Close") | color(theme::mint()));
        }
        content.push_back(text(""));
        return window(text(title) | color(theme::mint()) | bold, vbox(std::move(content))) |
            size(WIDTH, EQUAL, std::min(width - 4, 68)) | bgcolor(theme::panel()) | color(theme::text()) | clear_under | center;
    }
public:
    PlayerUI(Maker& songs, Bass& audio) : library(songs), player(audio) {
        refresh();
        if (!library.warnings.empty()) notify(library.warnings.front() + "  (? for activity)", true);
        for (const auto& warning : library.warnings) log.push_back(warning);
        while (log.size() > 7) log.pop_front();
    }
    ~PlayerUI() {
        cancel.store(true);
        if (download.valid()) {
            try { auto result = download.get(); std::error_code ignored; fs::remove_all(result.staging, ignored); } catch (...) {}
        }
    }
    void tick() {
        player.autochange();
        auto sample = player.spectrum();
        for (std::size_t i = 0; i < levels.size(); ++i) levels[i] = std::max(sample[i], levels[i] * 0.78f);
        if (lastPlaying != player.current()) {
            lastPlaying = player.current(); refresh(lastPlaying); error = false;
            if (!lastPlaying.empty()) {
                log.push_back("Playing " + library.song_table.at(lastPlaying).title);
                if (log.size() > 7) log.pop_front();
            }
        }
        if (busy() && download.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            std::optional<DownloadResult> result;
            try {
                result = download.get();
                if (cancel.load()) throw std::runtime_error("Download canceled.");
                commitDownload(library, *result); refresh(result->song.id);
                notify("Added " + result->song.title);
            } catch (const std::exception& e) { notify(e.what(), true); }
            if (result) { std::error_code ignored; fs::remove_all(result->staging, ignored); }
        }
    }
    ftxui::Element draw(int width, int height) {
        using namespace ftxui;
        if (width < 72 || height < 24) return vbox({text("RESONANCE") | bold | color(theme::mint()), text(""),
            text("Resize to at least 72 x 24."), text("Q quits. Downloads continue while resizing.")}) | center | bgcolor(theme::bg());
        auto selectedSong = selectedId();
        const Song* current = library.song_table.count(player.current()) ? &library.song_table.at(player.current()) : nullptr;
        long long total = 0; for (const auto& entry : library.song_table) total += entry.second.duration;
        auto masthead = hbox({text("  ◉  R E S O N A N C E") | bold | color(theme::mint()), text("   /   your music, in focus") | color(theme::muted()),
            filler(), text("LOCAL COLLECTION  ") | color(theme::violet())});
        auto tabs = hbox({text("  "), chip("1  Title", sort == "Title"), text("  "), chip("2  Duration", sort == "Duration"), text("  "),
            chip("3  Recent", sort == "Recent"), filler(), text(ascending ? "↑ Ascending  " : "↓ Descending  ") | color(theme::mint())});
        Elements rows;
        rowBoxes.resize(visible.size());
        for (int i = 0; i < static_cast<int>(visible.size()); ++i) {
            const auto& song = library.song_table.at(visible[i]);
            const bool missing = !fs::exists(library.root / "audioloc" / (song.id + ".mp3"));
            const bool playing = song.id == player.current();
            auto row = vbox({
                hbox({text(i == selected ? " ▎ " : "   ") | color(theme::mint()),
                    text(playing ? "♫ " : "  ") | color(theme::mint()), text(song.title) | bold | flex,
                    text("  " + formatTime(song.duration) + " ") | color(theme::muted())}),
                hbox({text("     "), text(song.artist + (missing ? "  · FILE MISSING" : "")) | color(missing ? theme::red() : theme::muted()) | flex})
            }) | reflect(rowBoxes[i]);
            if (i == selected) row = row | bgcolor(Color::RGB(29, 47, 53)) | focus;
            rows.push_back(row); rows.push_back(text(""));
        }
        if (rows.empty()) rows = {text(""), text(filter.empty() ? "Your collection starts here." : "No matching songs.") | bold,
            text(""), text(filter.empty() ? "Press F2 to download your first song." : "Press Esc to clear the filter.") | color(theme::muted())};
        auto list = vbox({
            hbox({text("  YOUR COLLECTION") | bold, filler(), text(std::to_string(visible.size()) + " tracks  ") | color(theme::muted())}),
            text(filter.empty() ? "  Every track, a little closer." : "  Filter: " + filter) | color(theme::muted()), text(""),
            vbox(std::move(rows)) | vscroll_indicator | yframe | flex
        }) | flex | bgcolor(theme::panel());
        int rightWidth = width >= 110 ? 36 : 30;
        auto details = vbox({
            text("  ON THE TURNTABLE") | color(theme::violet()) | bold,
            height >= 34 ? recordArt() : text(""),
            paragraph(current ? current->title : "Find your next favorite.") | size(HEIGHT, LESS_THAN, 3) | bold | color(theme::text()),
            paragraph(current ? current->artist : "Select a track from your collection.") | size(HEIGHT, LESS_THAN, 2) | color(theme::muted()),
            filler(), text(player.playing() ? "●  PLAYING" : player.paused() ? "Ⅱ  PAUSED" : "○  READY") | color(theme::mint()), text(""),
            height >= 32 ? spectrum(rightWidth - 4) : text(""), text(""),
            hbox({text("VOLUME") | color(theme::muted()), filler(), text(std::to_string(player.volume) + "%") | color(theme::gold())}),
            gauge(player.volume / 100.0f) | color(theme::gold()), text("")
        }) | size(WIDTH, EQUAL, rightWidth) | bgcolor(theme::panel());
        Elements body{list, text("  "), details};
        if (width >= 120) {
            auto sidebar = vbox({text(""), text(" YOUR SPACE") | color(theme::muted()), text(""),
                text(" ♫  All tracks") | color(theme::mint()) | bold, text(" /  Find a song") | color(theme::muted()),
                text(" +  F2 Download") | color(theme::muted()), filler(), text(" COLLECTION") | color(theme::muted()),
                text(" " + std::to_string(library.song_table.size()) + " tracks") | bold,
                text(" " + formatTime(total) + " total") | color(theme::muted()), text(""), text(" ?  Help & activity") | color(theme::violet()), text("")
            }) | size(WIDTH, EQUAL, 20);
            body.insert(body.begin(), text("  ")); body.insert(body.begin(), sidebar);
        }
        std::string status = player.message;
        if (busy()) { std::lock_guard<std::mutex> guard(progressMutex); status = "↓  " + progress + "  · Esc cancels"; }
        auto footer = vbox({text(""),
            hbox({text("  " + (current ? current->title : "Nothing playing")) | bold | flex,
                text(formatTime(player.position()) + " / " + formatTime(player.duration()) + "  ") | color(theme::mint())}),
            gauge(player.duration() > 0 ? static_cast<float>(std::clamp(player.position() / player.duration(), 0.0, 1.0)) : 0) |
                color(LinearGradient(theme::violet(), theme::mint())),
            text("  " + status) | color(error ? theme::red() : theme::muted()), divider(),
            text(width >= 110 ?
                "  Enter Play   Space Pause   N/P Skip   D/A Volume   F2 Add   R Remove   / Find   ? Help   Q Quit" :
                " Enter Play  Space Pause  N/P Skip  +/- Vol  F2 Add  R Remove  ? Help  Q Quit") | color(theme::mint())
        });
        auto page = vbox({text(""), masthead, text(""), tabs, text(""), hbox(std::move(body)) | flex, footer}) |
            color(theme::text()) | bgcolor(theme::bg());
        if (dialog != Dialog::None) return dbox({page, modal(width, height)});
        return page;
    }
    bool event(ftxui::Event e) {
        using namespace ftxui;
        try {
            if (dialog != Dialog::None) {
                if (e == Event::Escape) { dialog = Dialog::None; return true; }
                if (dialog == Dialog::Help) return true;
                if (dialog == Dialog::Delete) {
                    if (e == Event::Character('y') || e == Event::Character('Y')) { dialog = Dialog::None; removeSelected(); }
                    else if (e == Event::Character('n') || e == Event::Character('N')) dialog = Dialog::None;
                    return true;
                }
                if (e == Event::Return) {
                    if (dialog == Dialog::Search) { auto keep = selectedId(); filter = inputText; refresh(keep); dialog = Dialog::None; }
                    else startDownload();
                    return true;
                }
                return input->OnEvent(e);
            }
            if (e.is_character() && e.character().size() == 1 && e.character()[0] >= 'A' && e.character()[0] <= 'Z')
                e = Event::Character(static_cast<char>(e.character()[0] + 'a' - 'A'));
            if (e == Event::Escape) {
                if (busy()) { cancel.store(true); notify("Canceling download..."); }
                else if (!filter.empty()) { filter.clear(); refresh(); }
                return true;
            }
            if (e == Event::Character('?') || e == Event::F1) { dialog = Dialog::Help; return true; }
            if (e == Event::Character('/')) { open(Dialog::Search); return true; }
            if (e == Event::F2) { if (!busy()) open(Dialog::Download); else notify("A download is already running."); return true; }
            if (e == Event::F5) {
                if (busy()) { notify("Wait for the download before reloading."); return true; }
                auto keep = selectedId(); library.reload();
                if (!library.song_table.count(player.current())) player.clear();
                refresh(keep); notify(library.warnings.empty() ? "Library reloaded." : library.warnings.front(), !library.warnings.empty()); return true;
            }
            if (e == Event::ArrowUp || e == Event::ArrowDown || e == Event::PageUp || e == Event::PageDown || e == Event::Home || e == Event::End) {
                int step = e == Event::ArrowUp ? -1 : e == Event::ArrowDown ? 1 : e == Event::PageUp ? -8 : 8;
                if (!visible.empty()) selected = e == Event::Home ? 0 : e == Event::End ? static_cast<int>(visible.size()) - 1 :
                    std::clamp(selected + step, 0, static_cast<int>(visible.size()) - 1);
                return true;
            }
            if (e == Event::Return) { error = !player.playsong(selectedId()); return true; }
            if (e == Event::Character(' ') || e == Event::Character('s')) { player.toggle(selectedId()); error = false; return true; }
            if (e == Event::Character('n')) { player.next(); refresh(player.current()); return true; }
            if (e == Event::Character('p')) { player.prev(); refresh(player.current()); return true; }
            if (e == Event::ArrowLeft || e == Event::ArrowRight) { player.seek(e == Event::ArrowLeft ? -5 : 5); return true; }
            if (e == Event::Character('d') || e == Event::Character('+') || e == Event::Character('=')) { player.setvolume(player.volume + 5); return true; }
            if (e == Event::Character('a') || e == Event::Character('-')) { player.setvolume(player.volume - 5); return true; }
            if (e == Event::Character('1') || e == Event::Character('2') || e == Event::Character('3') || e == Event::Character('x')) {
                auto keep = selectedId();
                if (e == Event::Character('x')) ascending = !ascending;
                else sort = e == Event::Character('1') ? "Title" : e == Event::Character('2') ? "Duration" : "Recent";
                library.rebuild(sort, ascending); refresh(keep); return true;
            }
            if (e == Event::Character('r') || e == Event::Delete) {
                if (!selectedId().empty()) { deleteId = selectedId(); dialog = Dialog::Delete; } return true;
            }
            if (e.is_mouse()) {
                auto mouse = e.mouse();
                if (mouse.button == Mouse::WheelDown) { if (!visible.empty()) selected = std::min(selected + 1, static_cast<int>(visible.size()) - 1); return true; }
                if (mouse.button == Mouse::WheelUp) { selected = std::max(0, selected - 1); return true; }
                if (mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
                    for (int i = 0; i < static_cast<int>(rowBoxes.size()); ++i) if (rowBoxes[i].Contain(mouse.x, mouse.y)) { selected = i; return true; }
                }
            }
        } catch (const std::exception& exception) { notify(exception.what(), true); }
        return false;
    }
    void run() {
        using namespace ftxui;
        auto screen = ScreenInteractive::Fullscreen();
        auto renderer = Renderer([&] { auto size = Terminal::Size(); return draw(size.dimx, size.dimy); });
        auto component = CatchEvent(renderer, [&](Event e) {
            // Typing Q in text input must not quit the application.
            if (dialog == Dialog::None && (e == Event::Character('q') || e == Event::Character('Q') || e == Event::CtrlC)) {
                cancel.store(true); screen.ExitLoopClosure()(); return true;
            }
            return event(e);
        });
        Loop loop(&screen, component);
        while (!loop.HasQuitted()) {
            tick(); screen.PostEvent(Event::Custom); loop.RunOnce();
            std::this_thread::sleep_for(std::chrono::milliseconds(player.playing() ? 40 : 100));
        }
    }
};
