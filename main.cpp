#include <windows.h>
#include <iostream>
#include "Header_Files/ui.hpp"

int wmain(int argc, wchar_t** argv) {
    try {
        if (GetEnvironmentVariableW(L"WT_SESSION", nullptr, 0) > 0)
            ftxui::Terminal::SetColorSupport(ftxui::Terminal::Color::TrueColor);
        fs::path root = findProjectRoot();
        bool snapshot = false;
        fs::path destination;
        for (int i = 1; i < argc; ++i) {
            const std::wstring arg = argv[i];
            if (arg == L"--root" && i + 1 < argc) root = fs::path(argv[++i]);
            else if (arg == L"--snapshot" && i + 1 < argc) { snapshot = true; destination = fs::path(argv[++i]); }
            else if (arg == L"--help") {
                std::cout << "Resonance - Windows C++ music player\n"
                    "  --root PATH        Use a different library/project folder\n"
                    "  --snapshot FILE    Write a 120x40 ANSI UI preview without playing audio\n";
                return 0;
            } else throw std::runtime_error("Unknown/incomplete argument. Use --help.");
        }
        root = fs::absolute(root);
        Maker library(root);
        Bass player(library);
        PlayerUI ui(library, player);
        if (snapshot) {
            ftxui::Terminal::SetColorSupport(ftxui::Terminal::Color::TrueColor);
            auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(120), ftxui::Dimension::Fixed(40));
            ftxui::Render(screen, ui.draw(120, 40));
            std::ofstream output(destination, std::ios::binary); output << screen.ToString();
            if (!output) throw std::runtime_error("Cannot write snapshot.");
            return 0;
        }
        ui.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Resonance: " << error.what() << "\n"
            "Existing library files were not reset. Check Info_files/info.json or use --root PATH.\n";
        return 1;
    }
}
