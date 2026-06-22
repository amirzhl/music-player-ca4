#include "M3uLoader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "MusicLibrary.h"
#include "Playlist.h"
#include "Song.h"
#include "Utils.h"

namespace fs = std::filesystem;

std::vector<Playlist*> M3uLoader::loadAll(const std::string& directory,
                                          MusicLibrary& library) {
    std::vector<Playlist*> playlists;

    std::error_code ec;
    if (!fs::exists(directory, ec) || !fs::is_directory(directory, ec)) {
        std::cerr << "[warn] playlist directory not found: " << directory << "\n";
        return playlists;
    }

    // Collect the .m3u paths first, then load them in a stable (sorted) order.
    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::string ext = entry.path().extension().string();
        if (utils::iequals(ext, ".m3u")) {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());

    for (const std::string& f : files) {
        Playlist* playlist = loadFile(f, library);
        if (playlist != nullptr) {
            playlists.push_back(playlist);
        }
    }
    return playlists;
}

Playlist* M3uLoader::loadFile(const std::string& filePath, MusicLibrary& library) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[warn] cannot open playlist: " << filePath << "\n";
        return nullptr;
    }

    const std::string name = fs::path(filePath).stem().string();
    Playlist* playlist = new Playlist(name);

    std::string rawLine;
    while (std::getline(file, rawLine)) {
        const std::string line = utils::trim(rawLine);
        if (line.empty() || line[0] == '#') {
            continue; // blank line or M3U directive/comment
        }
        Song* song = library.getSong(line);
        if (song != nullptr) {
            playlist->addSong(song);
        }
        // Paths absent from the library are intentionally ignored.
    }
    return playlist;
}
