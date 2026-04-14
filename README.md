# Music Player

A terminal-based offline music player written in C++.

## Overview

Music Player is a lightweight, command-line music player application designed for offline music playback. It provides a simple and efficient way to play audio files directly from your terminal.

## Features

- **Terminal-Based Interface**: Simple command-line interface for easy navigation and control
- **Offline Playback**: Play music files locally without internet connection
- **Lightweight**: Efficient C++ implementation with minimal dependencies
- **Cross-Platform**: Supports Windows and other operating systems

## Technology Stack

- **Primary Language**: C++ (95.4%)
- **Supporting Language**: C (4.6%)
- **Audio Library**: BASS (bass.dll included)

## Project Structure
Music-player/ 
  ├── Header_Files/ # Header files for the project
      ├── ytdl/ # YouTube downloader module 
      ├── bass.h # BASS library header
      ├── bass.lib # BASS library object file
      ├── bassplayer.hpp # Bass player C++ header
      ├── downloader.hpp # Downloader utility C++ header
      ├── json.hpp # JSON parsing C++ header
      ├── maker.hpp # Maker utility C++ header
      └── preprocessing.hpp # Preprocessing C++ header (10 KB
  ├── Info_files/ # Information and documentation files
      ├── info.json # Stores info about all songs 
      ├── temp.json # Temproary json file for info of latest song
  ├── audioloc/ # Audio location/management module 
  ├── main.cpp # Main application entry point 
  ├── bass.dll # BASS audio library for Windows 
  ├── buffer.txt # Buffer file for processing 
  └── .gitignore # Git ignore rules


## Getting Started

### Prerequisites

- C++ compiler (supporting C++11 or later)
- BASS library (included as bass.dll for Windows)

### Installation

1. Clone the repository: git clone https://github.com/usaiddd/Music-player.git
2. Run the player.exe found in Music-player folder to enjoy the Terminal based Music Player!

## Key Components

bassplayer.hpp: Core audio playback functionality using BASS library
downloader.hpp: Audio file downloading capabilities
json.hpp: JSON data parsing and handling
preprocessing.hpp: Audio file preprocessing and optimization
ytdl/: YouTube audio download module
main.cpp: Main application logic and user interface
