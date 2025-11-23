#include "Header_Files/downloader.hpp"
#include "Header_Files/bassplayer.hpp"

int main(){
    system("cls");
    cout << "WELCOME TO OUR TERMINAL BASED MUSIC PLAYER"; 
    int n; 
    cout << "\nHow many songs do you want to download? "; 
    cin >> n; 
    cin.ignore(); 
    for (int i=0; i<n; i++){ 
        downloadSong(); 
        cout << "\n";
    }
    cout << "\nDownloads complete. Enjoy listening!";
    Sleep(2250);
    system("cls");

    Maker a;
    Bass B;

    string sortType = "Title"; 
    bool showMenu = true;
    bool ascending = true;

    B.playsong(); 

    while (true) {
        if (showMenu) {
            
            cout << "\n";
            cout << "PRESS Q TO QUIT.\n";
            cout << "PRESS N TO NEXT.\n";
            cout << "PRESS P TO PREVIOUS.\n";
            cout << "PRESS S TO PLAY/PAUSE.\n";
            cout << "PRESS D TO INCREASE VOLUME BY 5%.\n";
            cout << "PRESS A TO DECREASE VOLUME BY 5%.\n";
            cout << "PRESS R TO DELETE SONG.\n";
            cout << "PRESS 1 TO SORT BY TITLE (DEFAULT).\n";
            cout << "PRESS 2 TO SORT BY DURATION.\n";
            cout << "PRESS 3 TO SORT BY RECENTLY ADDED. \n";
            cout << "PRESS X TO CHANGE SORT ORDER (ASC/DESC). \n";
            cout << "-------------------------------\n";
            cout << "Volume: " << static_cast<int>(B.volume * 100) << "%";
            showMenu = false;
        }

        if (_kbhit()) {
            char x = toupper(_getch());
            if (x == 'Q') {
                cout << "\nThank you for listening! ";
                B.stop();
                BASS_Free();
                break;
            }
            else if (x == 'N') {
                B.next();
                if (B.over) break;
                showMenu = true; 
            } 
            else if (x == 'P') {
                B.prev();
                if (B.over) break;
                showMenu = true;
            } 
            else if (x == 'S') {
                B.toggle();
            } 
            else if (x == 'D') {
                B.setvolume(B.volume + 0.05f);
                cout << "\r" << string(30, ' ') << "\r";
                cout << "Volume: " << static_cast<int>(B.volume * 100) << "%" << flush;
            } 
            else if (x == 'A') {
                B.setvolume(B.volume - 0.05f);
                cout << "\r" << string(30, ' ') << "\r";
                cout << "Volume: " << static_cast<int>(B.volume * 100) << "%" << flush;
            }
            else if (x == '1' || x == '2' || x == '3') {
                if (x == '1') sortType = "Title";
                if (x == '2') sortType = "Duration";
                if (x == '3') sortType = "Recent"; 
                system("cls");
                B.rebuild(sortType, ascending);
                showMenu = true;
            }
            else if (x == 'X') {
                ascending = !ascending;
                B.rebuild(sortType, ascending);
                showMenu = true;
            }
            else if (x == 'R'){ 
                cout << "\nAre you sure you want to delete your current playing song? (yes/no): ";
                string ans;
                cin >> ans;
                for (auto &c : ans) c = tolower(c);

                if (ans == "yes" || ans == "y") {
                    ListNode* curr = B.getcur();
                    if (!curr) {
                        cout << "No current song to delete.\n";
                        showMenu = true;
                        continue;
                    }

                    Maker &mk = B.getm();
                    auto songInfoIt = mk.song_table.find(curr->data);

                    if (songInfoIt != mk.song_table.end()) {
                        json song = songInfoIt->second;
                        string title = song["title"].get<string>();

                        cout << "\nDeleting \"" << title << "\"...\n";
                        B.stop();
                        mk.deleteSong(title);
                        if (curr->next) {
                            B.setcur(curr->next);
                            B.playsong();
                        } 
                        else if (curr->prev) {
                            B.setcur(curr->prev);
                            B.playsong();
                        } 
                        else {
                            cout << "\nSong deleted. No more songs left in playlist.\n";
                            cout << "Thank you for listening! ";
                            B.stop(); 
                            B.setcur(nullptr);
                            BASS_Free();  
                            break; 
                        }
                    } 
                    else {
                        cout << "Could not find the song in the database.\n";
                    }
                } 
                else if (ans == "no" || ans == "n") {
                    cout << "Deletion canceled.\n";
                } 
                else {
                    cout << "Invalid input. Please type yes or no.\n";
                }

                showMenu = true;
            }
        }

        B.autochange();
        Sleep(500);
    }
    return 0;
}
