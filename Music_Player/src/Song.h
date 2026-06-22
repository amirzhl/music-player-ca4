#pragma once

#include <string>

// A single immutable song with all of its metadata.
// Encapsulation: every data member is private and exposed only through getters.
// A Song never performs any console I/O (separation of concerns).
class Song {
public:
    Song(std::string title,
         std::string artist,
         std::string album,
         std::string genre,
         int year,
         int durationSec,
         std::string filePath);

    const std::string& getTitle() const { return title_; }
    const std::string& getArtist() const { return artist_; }
    const std::string& getAlbum() const { return album_; }
    const std::string& getGenre() const { return genre_; }
    int getYear() const { return year_; }
    int getDuration() const { return durationSec_; }
    const std::string& getFilePath() const { return filePath_; }

    // Duration formatted as MM:SS.
    std::string getDurationString() const;

private:
    std::string title_;
    std::string artist_;
    std::string album_;
    std::string genre_;
    int year_;
    int durationSec_;
    std::string filePath_;
};
