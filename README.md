# Music Player

A terminal-based offline music player written in C++.

---

## Overview

Music Player is a lightweight, command-line music player application designed for offline music playback. It provides a simple and efficient way to play audio files directly from your terminal.

---

## Features

- Terminal-based interface for easy navigation and control  
- Offline playback (no internet required)  
- Lightweight and efficient C++ implementation  
- Cross-platform support (Windows + others)

---

## Technology Stack

- **Primary Language**: C++ (95.4%)  
- **Supporting Language**: C (4.6%)  
- **Audio Library**: BASS (bass.dll included)

---

## Project Structure

```bash
Music-player/
│
├── Header_Files/                # Header files for the project
│   ├── ytdl/                   # YouTube downloader module
│   ├── bass.h                  # BASS library header
│   ├── bass.lib                # BASS library object file
│   ├── bassplayer.hpp          # Audio playback logic
│   ├── downloader.hpp          # Downloader utility
│   ├── json.hpp                # JSON parsing
│   ├── maker.hpp               # Utility functions
│   └── preprocessing.hpp       # Preprocessing module
│
├── Info_files/                 # Information & metadata
│   ├── info.json               # Stores all songs info
│   └── temp.json               # Temporary latest song data
│
├── audioloc/                   # Audio file management
│
├── main.cpp                    # Entry point of application
├── bass.dll                    # BASS audio library (Windows)
├── buffer.txt                  # Buffer file for processing
└── .gitignore                  # Git ignore rules
```

---

## Getting Started

### Prerequisites

- C++ compiler (C++11 or later)
- BASS library (included for Windows)
- YT-DLP Module for Windows 

---

### Installation

```bash
git clone https://github.com/usaiddd/Music-player.git
cd Music-player
```

Run:

```bash
player.exe
```

---

## Key Components

- **bassplayer.hpp** → Handles audio playback using BASS  
- **downloader.hpp** → Manages audio downloads  
- **json.hpp** → Parses and manages JSON data  
- **preprocessing.hpp** → Optimizes audio before playback  
- **ytdl/** → YouTube audio downloader module  
- **main.cpp** → Core application logic  
