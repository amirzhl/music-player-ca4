#include "ConfigManager.h"

#include <fstream>
#include <iostream>

#include "Utils.h"

void ConfigManager::load(const std::string& path) {
    path_ = path;
    std::ifstream file(path);
    if (!file.is_open()) {
        // No settings yet: this is a normal first run.
        return;
    }

    std::string rawLine;
    while (std::getline(file, rawLine)) {
        const std::string line = utils::trim(rawLine);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue; // malformed line, ignore
        }
        const std::string key = utils::trim(line.substr(0, eq));
        const std::string value = utils::trim(line.substr(eq + 1));
        if (!key.empty()) {
            values_[key] = value;
        }
    }
}

void ConfigManager::save() const {
    save(path_);
}

void ConfigManager::save(const std::string& path) const {
    if (path.empty()) {
        return;
    }
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[warn] cannot write settings file: " << path << "\n";
        return;
    }
    for (const auto& kv : values_) {
        file << kv.first << "=" << kv.second << "\n";
    }
}

std::string ConfigManager::get(const std::string& key, const std::string& fallback) const {
    const auto it = values_.find(key);
    return it == values_.end() ? fallback : it->second;
}

void ConfigManager::set(const std::string& key, const std::string& value) {
    values_[key] = value;
}

bool ConfigManager::has(const std::string& key) const {
    return values_.find(key) != values_.end();
}
