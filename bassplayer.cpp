#include <iostream>
#include <string>
#include <conio.h>
#include "Header_Files/bass.h"
#include "Header_Files/maker.hpp"
using namespace std; 

class Bass{
private:  
  HSTREAM stream;
  Maker m;
  ListNode *cur = m.createAscendingList_Dur();
  string folder = "audioloc/";
public: 
  bool over = FALSE; 
  Bass(){ 
    if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
        cout << "BASS initialization failed!" << endl;
    } else {
        cout << "BASS initialized successfully!" << endl;
    }
    stream = 0;
  }
  void stop(){ 
    BASS_Stop();
  }
  void toggle(){ 
    if (!stream){
      cout << "No song loaded to play/pause." << endl;
      return;
    }
    DWORD state = BASS_ChannelIsActive(stream);
    if (state == BASS_ACTIVE_PLAYING) {
      if (BASS_ChannelPause(stream)) {
          cout << "Song PAUSED: " << cur->data << endl;
      }
    } else if (state == BASS_ACTIVE_PAUSED) {
      if (BASS_ChannelPlay(stream, FALSE)) {
          cout << "Song RESUMED: " << cur->data << endl;
      }
    }
  }
  void playsong(){ 
    if (!cur){ 
      cout << "No songs in playlist."; 
      return;
    }
    if (stream){ 
      BASS_StreamFree(stream);
    }
    string title = cur->data;
    string path = folder + title + ".mp3";
    stream = BASS_StreamCreateFile(FALSE, path.c_str(), 0,0,0);
    if (!stream){ 
      cout << "Failed to load song"; 
      return; 
    }
    cout << "Song playing: " << title << endl ;
    BASS_ChannelPlay(stream,FALSE);
  }
  void next(){ 
    if (!cur->next){ 
      cout << "Thank you for listening !";
      BASS_Stop();
      over = TRUE;
      return;
    }
    else{
      cur = cur->next; 
      playsong();
    }
  }
  void prev(){ 
    if (!cur->prev){ 
      cout << "Thank you for listening !";  
      BASS_Stop();
      over = TRUE; 
      return;
    }
    else{ 
      cur = cur->prev;
      playsong(); 
    }
  }
  void autochange(){ 
    if (!stream) return; 
    
    DWORD status = BASS_ChannelIsActive(stream);
    if (status == BASS_ACTIVE_STOPPED){ 
      cout << "\nSong finished. Auto-advancing to next track." << endl;
       next(); 
    }
  }
  ~Bass() { 
    BASS_Free();
  }
}; 

int main(){ 
  Maker playlist; 
  Bass B;
  B.playsong();
  char x; 
  while (true) {
    if (_kbhit()) {
      cout << "\n"; 
      cout << "PRESS Q TO QUIT. " << endl; 
      cout << "PRESS N TO NEXT. " << endl; 
      cout << "PRESS P TO PREVIOUS. " << endl; 
      cout << "PRESS S TO PLAY/PAUSE. " << endl;
      char x = toupper(_getch());

      if (x == 'Q') {
          B.stop();
          break;
      } 
      else if (x == 'N') {
          B.next();
          if (B.over) break;
      } 
      else if (x == 'P') {
          B.prev();
          if (B.over) break;
      } 
      else if (x == 'S') {
          B.toggle();
      }
    }
    B.autochange();
    Sleep(500);
  }
}