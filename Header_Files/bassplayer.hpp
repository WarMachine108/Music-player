#include <iostream>
#include <string>
#include <conio.h>
#include "bass.h"
#include "maker.hpp"
using namespace std; 

#ifndef BASSPLAYER_HPP
#define BASSPLAYER_HPP

class Bass{ 
private:
  HSTREAM stream;
  Maker m;
  ListNode *cur;
  string folder;
public: 
  bool over; 
  float volume; 
  Bass(); 
  void stop(); 
  void toggle(); 
  void playsong(); 
  void get_title(); 
  void next(); 
  void prev(); 
  void autochange(); 
  void setvolume(float v); 
  ~Bass();
};

#endif

Bass::Bass() {
    cur = m.createAscendingList_Dur();
    folder = "audioloc/";
    over = FALSE;
    volume = 0.50f;
    if (!BASS_Init(-1, 44100, 0, 0, NULL))
        cout << "BASS initialization failed!\n";
    else
        cout << "BASS initialized successfully!\n";
    stream = 0;
}

void Bass::stop() { BASS_Stop(); }

void Bass::toggle() {
    if (!stream) {
        cout << "No song loaded to play/pause.\n";
        return;
    }
    DWORD state = BASS_ChannelIsActive(stream);
    if (state == BASS_ACTIVE_PLAYING) {
        if (BASS_ChannelPause(stream))
            cout << "Song PAUSED: " << cur->data << endl;
    } else if (state == BASS_ACTIVE_PAUSED) {
        if (BASS_ChannelPlay(stream, FALSE))
            cout << "Song RESUMED: " << cur->data << endl;
    }
}

void Bass::playsong() {
    if (!cur) { cout << "No songs in playlist.\n"; return; }
    if (stream) BASS_StreamFree(stream);
    string path = "audioloc/" + cur->data + ".mp3";
    stream = BASS_StreamCreateFile(FALSE, path.c_str(), 0, 0, 0);
    if (!stream) { cout << "Failed to load song\n"; return; }
    BASS_ChannelSetAttribute(stream, BASS_ATTRIB_VOL, volume);
    BASS_ChannelPlay(stream, FALSE);
}

void Bass::get_title() { cout << "Song playing: " << cur->data << endl; }

void Bass::next() {
    if (!cur->next) {
        cout << "Thank you for listening!\n";
        BASS_Stop();
        over = TRUE;
        return;
    }
    cur = cur->next;
    playsong();
}

void Bass::prev() {
    if (!cur->prev) {
        cout << "Thank you for listening!\n";
        BASS_Stop();
        over = TRUE;
        return;
    }
    cur = cur->prev;
    playsong();
}

void Bass::autochange() {
    if (!stream) return;
    DWORD status = BASS_ChannelIsActive(stream);
    if (status == BASS_ACTIVE_STOPPED) {
        cout << "\nSong finished. Auto-advancing to next track.\n";
        next();
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

Bass::~Bass() { BASS_Free(); }

