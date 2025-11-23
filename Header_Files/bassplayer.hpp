#include <iostream>
#include <string>
#include <conio.h>
#include "bass.h"
#include "maker.hpp"
using namespace std; 

#ifndef BASSPLAYER_HPP
#define BASSPLAYER_HPP
static bool bass_initialized = false;

class Bass{ 
private:
    HSTREAM stream;
    Maker m;
    ListNode *cur;
    string folder;
    string sortType = "Title";
    bool ascending = true;
    bool waiting_for_next = false;
public:
    bool over; 
    float volume; 
    Bass();
    ListNode* getcur();
    ListNode* setcur(ListNode* temp); 
    Maker& getm(); 
    void stop(); 
    void toggle(); 
    void playsong(); 
    void next(); 
    void prev(); 
    void autochange(); 
    void setvolume(float v);
    void showAlbumArt();
    void rebuild(string sortType, bool ascending);
    void QueueSong(const string& s); 
};

#endif

Bass::Bass(){
    cur = m.createAscendingList();
    folder = "audioloc/";
    over = FALSE;
    volume = 0.50f;

    if (!bass_initialized) {
        if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
            DWORD err = BASS_ErrorGetCode();
            cout << "BASS initialization failed! Error code: " << err << endl;
        } else {
            cout << "BASS initialized successfully!" << endl;
            bass_initialized = true;
        }
    }
    stream = 0;
}

Maker& Bass::getm(){
    return m; 
}

ListNode* Bass::getcur(){ 
    return cur; 
}

ListNode* Bass::setcur(ListNode* temp){ 
    cur = temp; 
}

void Bass::stop() { 
    if (stream) {
        BASS_ChannelStop(stream);
        BASS_StreamFree(stream);
        stream = 0;
    }
}

void Bass::toggle() {
    if (!stream) {
        cout << "No song loaded to play/pause.\n";
        return;
    }
    DWORD state = BASS_ChannelIsActive(stream);
    if (state == BASS_ACTIVE_PLAYING) {
        if (BASS_ChannelPause(stream))
            cout << "\nSong PAUSED " << endl;
    } else if (state == BASS_ACTIVE_PAUSED) {
        if (BASS_ChannelPlay(stream, FALSE))
            cout << "Song RESUMED " << endl;
    }
}

void Bass::playsong() {
    if (!cur) {
        cout << "No songs in playlist.\n";
        return;
    }
    if (!bass_initialized) {
        if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
            cerr << "BASS init failed: " << BASS_ErrorGetCode() << "\n";
            return;
        }
        else{ 
            bass_initialized = true; 
        }
    }

    if (stream) {
        BASS_ChannelStop(stream);
        BASS_StreamFree(stream);
        stream = 0;
    }

    string path = "audioloc/" + cur->data + ".mp3";
    stream = BASS_StreamCreateFile(FALSE, path.c_str(), 0, 0, 0);
    if (!stream) {
        cerr << " Failed to start playback. BASS error code: "
             << BASS_ErrorGetCode() << endl;
        return;
    }
    system("cls");
    auto songInfoIt = m.song_table.find(cur->data);
    if (songInfoIt != m.song_table.end()) {
        json song = songInfoIt->second;
        cout << "Now Playing: " << song["title"].get<string>() << "\n";
    } else {
        cout << "Now Playing: " << cur->data << "\n";
    }
    cout << "Sorted by: " << sortType << endl;
    cout << "Ordered by: " << (ascending ? "Ascending" : "Descending") << endl;
    showAlbumArt();
    BASS_ChannelSetAttribute(stream, BASS_ATTRIB_VOL, volume);
    BASS_ChannelPlay(stream, FALSE);
    waiting_for_next = false;
}


void Bass::next() {
    if (!cur->next) {
        cout << "\nThank you for listening!\n";
        BASS_Stop();
        over = TRUE;
        return;
    }
    cur = cur->next;
    waiting_for_next = false;
    playsong();
}

void Bass::prev() {
    if (!cur->prev) {
        cout << "\nThank you for listening!\n";
        BASS_Stop();
        over = TRUE;
        return;
    }
    cur = cur->prev;
    playsong();
    waiting_for_next = false;
}

void Bass::autochange() {
    if (!stream) return;
    DWORD status = BASS_ChannelIsActive(stream);
    if (status == BASS_ACTIVE_STOPPED && !waiting_for_next) {
        waiting_for_next = true;
        cout << "\nSong finished. Auto-advancing to next track.\n";
        next();
    }
    else if (status == BASS_ACTIVE_PLAYING) {
        waiting_for_next = false;
    }
}

void Bass::setvolume(float v) {
    if (!stream) {
        cout << "No active stream to set volume.\n";
        return;
    }
    if (v < 0.05f) v = 0.00f;
    if (v > 0.95f) v = 1.00f;
    volume = v;
    BASS_ChannelSetAttribute(stream, BASS_ATTRIB_VOL, volume);
}

void Bass::showAlbumArt(){ 
    string imgPath = folder + cur->data + ".webp";
    string cmd = 
        "powershell -Command \"Import-Module Sixel; "
        "ConvertTo-Sixel -Path '" + imgPath + "'\"";
    system(cmd.c_str());
    cout << "\n";
}

void Bass::rebuild(string sortType, bool ascending) {
    this->sortType = sortType;
    this->ascending = ascending;
    waiting_for_next = false;
    over = false;

    if (sortType == "Title")
        cur = ascending ? m.createAscendingList() : m.createDescendingList();
    else if (sortType == "Duration")
        cur = ascending ? m.createAscendingList_Dur() : m.createDescendingList_Dur();
    else if (sortType == "Recent")
        cur = ascending ? m.createAscendingList_rec(): m.createDescendingList_rec();

    if (!cur) {
        cout << "Playlist empty or failed to build.\n";
        return;
    }
    stop();
    playsong();
}

void QueueSong(const string& s); 