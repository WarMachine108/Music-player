#include "json.hpp"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>

using json = nlohmann::json;
using namespace std;

#ifndef DOWNLOADER_HPP
#define DOWNLOADER_HPP
int downloadSong();
    //further declarations if needed

#endif // DOWNLOADER_HPP

int downloadSong()
{
    cout << "Enter song title with artist name : ";
    string mid, first = "yt-dlp \"ytsearch1:", last = " \" --get-id 1 >> buffer.txt", fin = "", errorHandler;
    #ifdef _WIN32
        errorHandler = " 2>nul";
    #else
        errorHandler = " 2>/dev/null";
    #endif
    getline(cin, mid);
    fin = first + mid + last + errorHandler;
    system(fin.c_str());

    string firstLine;
    try {
        ifstream inFile("buffer.txt");
        getline(inFile, firstLine);
        inFile.close();
        ofstream outFile("buffer.txt", ios::trunc);
        outFile.close();
    }
    catch(...){

    }
    
    string videoUrl="https://www.youtube.com/watch?v=" + firstLine;
    string commandtojson = "yt-dlp \"" + videoUrl +
                           "\" --print \"{\\\"title\\\": \\\"%(title)s\\\", "
                           "\\\"uploader\\\": \\\"%(uploader)s\\\", "
                           "\\\"duration\\\": \\\"%(duration)s\\\"}\" > Info_files/temp.json" + errorHandler;
    
    string audioPath = "audioloc/" + firstLine + ".mp3";
    string thumbPath = "album_art/" + firstLine + ".webp";
    string commandtodownload = "yt-dlp -x -q --write-thumbnail --audio-quality 0 --audio-format mp3 "
        "-o \"audioloc/%(id)s.%(ext)s\" \"" + videoUrl + "\"" + errorHandler;

    cout << "Running command..." << endl;
    int result1 = system(commandtojson.c_str());
    int result2 = system(commandtodownload.c_str());

    if (result1 != 0 || result2 != 0)
    {
        if (result1 != 0) cerr << "JSON command failed!" << endl;
        if (result2 != 0) cerr << "DOWNLOAD command failed!" << endl;
        return 1;
    }
    else{
        cout<<"Downloaded Successfully ";
    }

    ifstream tempFile("Info_files/temp.json");
    if (!tempFile)
    {
        cerr << "Failed to open temp.json" << endl;
        return 1;
    }
    json newEntry;
    tempFile >> newEntry;
    tempFile.close();
    newEntry["id"] = firstLine; 

    json allData;
    ifstream infoFile("Info_files/info.json");
    if (infoFile && infoFile.peek() != ifstream::traits_type::eof())
    {
        try
        {
            infoFile >> allData;
        }
        catch (...)
        {
            cerr << "Corrupted info.json, resetting..." << endl;
            allData = json::array();
        }
    }
    else
    {
        allData = json::array();
        if (infoFile) infoFile.close();
    }

    allData.push_back(newEntry);

    ofstream jsonOutFile("Info_files/info.json");
    jsonOutFile << setw(4) << allData << endl;
    jsonOutFile.close();
    return 0;
}