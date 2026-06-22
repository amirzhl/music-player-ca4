#include "MusicLibrary.h"

#include "Song.h"
#include "Utils.h"

MusicLibrary::~MusicLibrary() {
    // The one and only place Song objects are released.
    for (Song* song : songs_) {
        delete song;
    }
    songs_.clear();
    byPath_.clear();
}

void MusicLibrary::addSong(Song* song) {
    if (song == nullptr) {
        return;
    }
    songs_.push_back(song);
    byPath_[song->getFilePath()] = song;
}

Song* MusicLibrary::getSong(const std::string& filePath) const {
    const auto it = byPath_.find(filePath);
    return it == byPath_.end() ? nullptr : it->second;
}

std::vector<Song*> MusicLibrary::filterByArtist(const std::vector<Song*>& songs,
                                                const std::string& artist) {
    std::vector<Song*> result;
    for (Song* song : songs) {
        if (song != nullptr && utils::iequals(song->getArtist(), artist)) {
            result.push_back(song);
        }
    }
    return result;
}

std::vector<Song*> MusicLibrary::filterByAlbum(const std::vector<Song*>& songs,
                                               const std::string& album) {
    std::vector<Song*> result;
    for (Song* song : songs) {
        if (song != nullptr && utils::iequals(song->getAlbum(), album)) {
            result.push_back(song);
        }
    }
    return result;
}

std::vector<Song*> MusicLibrary::search(const std::vector<Song*>& songs,
                                        const std::string& query) {
    std::vector<Song*> result;
    for (Song* song : songs) {
        if (song == nullptr) {
            continue;
        }
        if (utils::icontains(song->getTitle(), query) ||
            utils::icontains(song->getArtist(), query) ||
            utils::icontains(song->getAlbum(), query)) {
            result.push_back(song);
        }
    }
    return result;
}
