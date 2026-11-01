#pragma once
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "storage.hpp"
#include "preprocessing.hpp"

struct ListNode {
    std::string data;
    ListNode* next = nullptr;
    ListNode* prev = nullptr;
    explicit ListNode(std::string id) : data(std::move(id)) {}
};

// Hash tables: lookup. AVL trees: sorted indexes. Doubly linked list: playback.
class Maker {
    using TitleKey = std::pair<std::string, std::string>;
    AVL<TitleKey> title;
    AVL<int> duration;
    std::vector<std::string> recent_add;
    ListNode* head = nullptr;
    json original = json::array();
    std::string sort_ = "Title";
    bool ascending_ = true;
    void clearList() { while (head) { auto* next = head->next; delete head; head = next; } }
    void index(const Song& s) {
        song_table.emplace(s.id, s); title.insert({folded(s.title), s.id});
        if (!duration_table.count(s.duration)) duration.insert(s.duration);
        duration_table[s.duration].push_back(s.id); recent_add.push_back(s.id);
    }
public:
    fs::path root;
    std::unordered_map<std::string, Song> song_table;
    std::unordered_map<int, std::vector<std::string>> duration_table;
    std::vector<std::string> warnings;
    bool readOnly = false;
    explicit Maker(fs::path directory) : root(std::move(directory)) { reload(); }
    ~Maker() { clearList(); }
    Maker(const Maker&) = delete;
    Maker& operator=(const Maker&) = delete;
    void reload() {
        auto data = readMetadata(root / "Info_files/info.json");
        std::vector<Song> parsed;
        std::unordered_set<std::string> seen;
        std::vector<std::string> notes;
        bool invalid = false;
        for (std::size_t i = 0; i < data.size(); ++i) {
            try {
                auto s = Song::parse(data[i]);
                if (!seen.insert(s.id).second) { notes.push_back("Duplicate ID ignored: " + s.id); continue; }
                parsed.push_back(std::move(s));
            } catch (const std::exception& e) { notes.push_back("Entry " + std::to_string(i + 1) + ": " + e.what()); invalid = true; }
        }
        clearList(); title.clear(); duration.clear(); song_table.clear(); duration_table.clear(); recent_add.clear();
        original = data; readOnly = invalid; warnings = std::move(notes);
        for (const auto& s : parsed) index(s);
        rebuild(sort_, ascending_);
    }
    std::vector<std::string> orderedIds() const {
        std::vector<std::string> ids;
        for (auto* n = head; n; n = n->next) ids.push_back(n->data);
        return ids;
    }
    ListNode* find(const std::string& id) const {
        for (auto* n = head; n; n = n->next) if (n->data == id) return n;
        return nullptr;
    }
    void rebuild(const std::string& sort, bool asc) {
        sort_ = sort; ascending_ = asc;
        std::vector<std::string> ids;
        if (sort == "Title") {
            std::vector<TitleKey> keys; title.displayinorder(keys);
            for (const auto& k : keys) ids.push_back(k.second);
        } else if (sort == "Duration") {
            std::vector<int> keys; duration.displayinorder(keys);
            for (int k : keys) {
                auto group = duration_table.at(k);
                std::sort(group.begin(), group.end(), [&](const auto& a, const auto& b) {
                    return TitleKey{folded(song_table.at(a).title), a} < TitleKey{folded(song_table.at(b).title), b};
                });
                ids.insert(ids.end(), group.begin(), group.end());
            }
        } else ids = recent_add;
        if (!asc) std::reverse(ids.begin(), ids.end());
        clearList(); ListNode* tail = nullptr;
        for (const auto& id : ids) {
            auto* n = new ListNode(id); n->prev = tail;
            if (tail) tail->next = n; else head = n;
            tail = n;
        }
    }
    void addSong(const Song& s) {
        if (readOnly) throw std::runtime_error("Repair invalid metadata entries before editing.");
        if (song_table.count(s.id)) throw std::runtime_error("Song already in library.");
        auto updated = original; updated.push_back(s.record);
        writeMetadata(root / "Info_files/info.json", original, updated);
        original = std::move(updated); index(s); rebuild(sort_, ascending_);
    }
    std::string deleteSong(const std::string& id) {
        if (readOnly) throw std::runtime_error("Repair invalid metadata entries before editing.");
        const auto s = song_table.at(id);
        json updated = json::array();
        for (const auto& record : original) if (record.at("id") != id) updated.push_back(record);
        // A failed metadata commit leaves indexes and media untouched.
        writeMetadata(root / "Info_files/info.json", original, updated); original = std::move(updated);
        title.erase({folded(s.title), id});
        auto& group = duration_table.at(s.duration);
        group.erase(std::remove(group.begin(), group.end(), id), group.end());
        if (group.empty()) { duration_table.erase(s.duration); duration.erase(s.duration); }
        recent_add.erase(std::remove(recent_add.begin(), recent_add.end(), id), recent_add.end()); song_table.erase(id);
        if (auto* n = find(id)) {
            if (n->prev) n->prev->next = n->next; else head = n->next;
            if (n->next) n->next->prev = n->prev;
            delete n;
        }
        std::string warning;
        for (const auto* ext : {".mp3", ".webp", ".jpg", ".png"}) {
            std::error_code error; fs::remove(root / "audioloc" / (id + ext), error);
            if (error) warning = "Removed from library; a media file could not be removed (it may be open elsewhere).";
        }
        return warning;
    }
};
