#pragma once

#include <map>
#include <string>

// Loads and saves the simple key=value settings file (Data/settings.cfg).
// Recognised keys: active_playlist, playback_mode, last_song.
class ConfigManager {
public:
    // Reads the file if it exists. Missing file is not an error (fresh start).
    void load(const std::string& path);

    // Persists current values. Uses the path remembered from load() when no
    // explicit path is given.
    void save() const;
    void save(const std::string& path) const;

    std::string get(const std::string& key, const std::string& fallback = "") const;
    void set(const std::string& key, const std::string& value);
    bool has(const std::string& key) const;

private:
    std::map<std::string, std::string> values_;
    std::string path_;
};
