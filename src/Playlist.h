#pragma once

#include <cstddef>
#include <string>
#include <vector>

class Song;

// An ordered, read-only collection of Song pointers.
//
// IMPORTANT (memory-ownership policy): a Playlist NEVER owns the Song objects it
// points to. The single owner of every Song is MusicLibrary, which is the only
// class allowed to delete them. Therefore Playlist's destructor must NOT delete
// the songs it references.
class Playlist {
public:
    explicit Playlist(std::string name);
    ~Playlist() = default; // intentionally does NOT free the Song pointers

    const std::string& getName() const { return name_; }
    const std::vector<Song*>& getSongs() const { return songs_; }

    std::size_t size() const { return songs_.size(); }
    bool empty() const { return songs_.empty(); }

    // Returns nullptr if the index is out of range.
    Song* getSong(std::size_t index) const;

    // Appends a song pointer (used by the M3U loader and the optional
    // "add song to playlist" bonus). Null pointers are ignored.
    void addSong(Song* song);

private:
    std::string name_;
    std::vector<Song*> songs_; // non-owning pointers
};
