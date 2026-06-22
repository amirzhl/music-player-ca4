#pragma once

#include <map>
#include <string>
#include <vector>

class Song;

// The complete catalogue of songs and the SOLE owner of every Song object.
//
// Dynamic-memory policy: songs are allocated with `new` (by the CSV loader) and
// handed to addSong(); MusicLibrary's destructor is the only place that frees
// them with `delete`. Search and filter helpers live here per the project spec.
class MusicLibrary {
public:
    MusicLibrary() = default;
    ~MusicLibrary();

    // Non-copyable: it owns raw pointers.
    MusicLibrary(const MusicLibrary&) = delete;
    MusicLibrary& operator=(const MusicLibrary&) = delete;

    // Takes ownership of `song`.
    void addSong(Song* song);

    const std::vector<Song*>& getAllSongs() const { return songs_; }
    std::size_t size() const { return songs_.size(); }

    // Look up a song by its (exact) file path. Returns nullptr if absent.
    Song* getSong(const std::string& filePath) const;

    // --- Filtering / searching helpers -------------------------------------
    // These operate on an arbitrary set of songs (e.g. the active playlist) so
    // the same logic backs both library-wide and playlist-scoped operations.
    static std::vector<Song*> filterByArtist(const std::vector<Song*>& songs,
                                              const std::string& artist);
    static std::vector<Song*> filterByAlbum(const std::vector<Song*>& songs,
                                             const std::string& album);
    // Case-insensitive match against title, artist or album.
    static std::vector<Song*> search(const std::vector<Song*>& songs,
                                     const std::string& query);

private:
    std::vector<Song*> songs_;            // owning pointers
    std::map<std::string, Song*> byPath_; // fast path -> Song lookup index
};
