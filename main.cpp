#include "Header_Files/downloader.hpp"
#include "Header_Files/bassplayer.hpp"

int main(){
    Maker a;
    Bass B;
    B.playsong();
    bool showMenu = true;
    while (true) {
    if (showMenu) {
        system("cls");
        B.get_title();
        cout << "\n";
        cout << "PRESS Q TO QUIT.\n";
        cout << "PRESS N TO NEXT.\n";
        cout << "PRESS P TO PREVIOUS.\n";
        cout << "PRESS S TO PLAY/PAUSE.\n";
        cout << "PRESS D TO INCREASE VOLUME BY 5%.\n";
        cout << "PRESS A TO DECREASE VOLUME BY 5%.\n";
        cout << "-------------------------------\n";
        cout << "Volume: " << static_cast<int>(B.volume * 100) << "%" << endl;
        showMenu = false;
    }

    if (_kbhit()) {
        char x = toupper(_getch());
        showMenu = true;

        if (x == 'Q') {
            B.stop();
            break;
        } else if (x == 'N') {
            B.next();
            if (B.over) break;
        } else if (x == 'P') {
            B.prev();
        } else if (x == 'S') {
            B.toggle();
        } else if (x == 'D') {
            B.setvolume(B.volume + 0.05f);
        } else if (x == 'A') {
            B.setvolume(B.volume - 0.05f);
        }
    }

    B.autochange();
    Sleep(500);
    }
}

