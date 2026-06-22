#pragma once

#include <string>

class MusicLibrary;

// Reads the central metadata CSV (Data/library.csv) once at startup and
// populates a MusicLibrary with newly-allocated Song objects.
//
// Robustness: a row with a missing/invalid field is skipped with a warning and
// loading continues. A missing/unreadable file is reported through an exception
// so the caller can handle the I/O error (try/catch).
class CsvLoader {
public:
    // Returns the number of songs successfully loaded.
    // Throws std::runtime_error if the file cannot be opened.
    int load(const std::string& path, MusicLibrary& library);
};
