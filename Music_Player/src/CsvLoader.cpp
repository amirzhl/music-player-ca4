#include "CsvLoader.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "MusicLibrary.h"
#include "Song.h"
#include "Utils.h"

namespace {
// Expected column order:
// title,artist,album,genre,year,duration_sec,file_path
constexpr std::size_t kExpectedFields = 7;
} // namespace

int CsvLoader::load(const std::string& path, MusicLibrary& library) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("CsvLoader: cannot open metadata file: " + path);
    }

    int loaded = 0;
    int lineNumber = 0;
    bool headerSkipped = false;
    std::string rawLine;

    while (std::getline(file, rawLine)) {
        ++lineNumber;
        const std::string line = utils::trim(rawLine);

        // Skip blank lines and comment lines.
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::vector<std::string> fields = utils::splitCsvLine(line);

        // Skip the header row (first non-comment line that starts with "title").
        if (!headerSkipped) {
            headerSkipped = true;
            if (!fields.empty() && utils::iequals(utils::trim(fields[0]), "title")) {
                continue;
            }
            // Otherwise there is no header; fall through and treat as data.
        }

        if (fields.size() < kExpectedFields) {
            std::cerr << "[warn] line " << lineNumber
                      << ": expected " << kExpectedFields << " fields, got "
                      << fields.size() << " -- row skipped\n";
            continue;
        }

        const std::string title = utils::trim(fields[0]);
        const std::string artist = utils::trim(fields[1]);
        const std::string album = utils::trim(fields[2]);
        const std::string genre = utils::trim(fields[3]);
        const std::string yearStr = utils::trim(fields[4]);
        const std::string durationStr = utils::trim(fields[5]);
        const std::string filePath = utils::trim(fields[6]);

        if (title.empty() || artist.empty() || filePath.empty()) {
            std::cerr << "[warn] line " << lineNumber
                      << ": empty title/artist/file_path -- row skipped\n";
            continue;
        }

        int year = 0;
        int duration = 0;
        try {
            year = std::stoi(yearStr);
            duration = std::stoi(durationStr);
        } catch (const std::exception&) {
            std::cerr << "[warn] line " << lineNumber
                      << ": non-numeric year/duration -- row skipped\n";
            continue;
        }

        if (duration < 0) {
            std::cerr << "[warn] line " << lineNumber
                      << ": negative duration -- row skipped\n";
            continue;
        }

        library.addSong(new Song(title, artist, album, genre, year, duration, filePath));
        ++loaded;
    }

    return loaded;
}
