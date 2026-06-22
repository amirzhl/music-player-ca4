#pragma once

#include <string>
#include <vector>

class MusicLibrary;
class Playlist;

// Loads every *.m3u file found in a directory and builds one Playlist per file.
//
// Each path inside an .m3u must match a Song's file_path in the library; paths
// that are not present in the library are silently ignored (read-only
// playlists). The returned Playlist pointers are owned by the caller
// (Application), which is responsible for deleting them. The Song pointers they
// reference remain owned by MusicLibrary.
class M3uLoader {
public:
    std::vector<Playlist*> loadAll(const std::string& directory,
                                   MusicLibrary& library);

private:
    Playlist* loadFile(const std::string& filePath, MusicLibrary& library);
};
