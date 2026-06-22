#include "Song.h"

#include <utility>

#include "Utils.h"

Song::Song(std::string title,
           std::string artist,
           std::string album,
           std::string genre,
           int year,
           int durationSec,
           std::string filePath)
    : title_(std::move(title)),
      artist_(std::move(artist)),
      album_(std::move(album)),
      genre_(std::move(genre)),
      year_(year),
      durationSec_(durationSec),
      filePath_(std::move(filePath)) {}

std::string Song::getDurationString() const {
    return utils::formatDuration(durationSec_);
}
