#pragma once

#include <cstddef>
#include <string>
#include <vector>

class MusicLibrary;
class Playlist;
class Player;
class ConfigManager;
class UIRenderer;
class InputHandler;

enum class FilterType { None, Artist, Album };

// A filter request passed from the Filter screen back to the Playlist view.
struct PendingFilter {
    FilterType type = FilterType::None;
    std::string value;
};

// Shared, dependency-injected context handed to every Screen. Holding the
// subsystems by reference here (rather than in globals) keeps the design free of
// mutable global state while still letting screens collaborate.
class AppContext {
public:
    AppContext(MusicLibrary& library,
               std::vector<Playlist*>& playlists,
               Player& player,
               ConfigManager& config,
               UIRenderer& ui,
               InputHandler& input)
        : library(library),
          playlists(playlists),
          player(player),
          config(config),
          ui(ui),
          input(input) {}

    MusicLibrary& library;
    std::vector<Playlist*>& playlists;
    Player& player;
    ConfigManager& config;
    UIRenderer& ui;
    InputHandler& input;

    int activePlaylistIndex = -1;
    PendingFilter pendingFilter;

    Playlist* activePlaylist() const {
        if (activePlaylistIndex < 0 ||
            activePlaylistIndex >= static_cast<int>(playlists.size())) {
            return nullptr;
        }
        return playlists[static_cast<std::size_t>(activePlaylistIndex)];
    }
};
