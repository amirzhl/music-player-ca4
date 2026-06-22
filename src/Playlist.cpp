#include "Playlist.h"

#include <utility>

#include "Song.h"

Playlist::Playlist(std::string name) : name_(std::move(name)) {}

Song* Playlist::getSong(std::size_t index) const {
    if (index >= songs_.size()) {
        return nullptr;
    }
    return songs_[index];
}

void Playlist::addSong(Song* song) {
    if (song != nullptr) {
        songs_.push_back(song);
    }
}
