#include "PlaylistListScreen.h"

#include <string>

#include "AppContext.h"
#include "ConfigManager.h"
#include "InputHandler.h"
#include "Player.h"
#include "Playlist.h"
#include "UIRenderer.h"

void PlaylistListScreen::onEnter() {
    // Start the highlight on the currently active playlist (or the first one).
    selected_ = ctx_.activePlaylistIndex >= 0 ? ctx_.activePlaylistIndex : 0;
    const int count = static_cast<int>(ctx_.playlists.size());
    if (selected_ >= count) selected_ = count - 1;
    if (selected_ < 0) selected_ = 0;
}

void PlaylistListScreen::render() {
    UIRenderer& ui = ctx_.ui;
    ui.header("PLAYLISTS");

    if (ctx_.playlists.empty()) {
        ui.boxLine("No playlists found in Data/Playlists.");
    } else {
        for (std::size_t i = 0; i < ctx_.playlists.size(); ++i) {
            Playlist* pl = ctx_.playlists[i];
            const bool active = (static_cast<int>(i) == ctx_.activePlaylistIndex);
            std::string line = "[" + std::to_string(i + 1) + "] " + pl->getName() +
                               "  (" + std::to_string(pl->size()) + " songs)";
            if (active) {
                line += "   * active";
            }
            if (static_cast<int>(i) == selected_) {
                ui.boxSelected(line);
            } else {
                ui.boxLine(line);
            }
        }
    }
    ui.boxSeparator();
    ui.boxLine("[Up/Down] move   [Enter] set active   [num] quick pick");
    ui.boxLine("[0] Back");
    ui.boxBottom();
}

void PlaylistListScreen::activate(int index) {
    const int count = static_cast<int>(ctx_.playlists.size());
    if (index < 0 || index >= count) {
        return;
    }
    selected_ = index;
    if (index == ctx_.activePlaylistIndex) {
        return; // already active
    }
    ctx_.activePlaylistIndex = index;
    Playlist* pl = ctx_.activePlaylist();
    if (pl != nullptr) {
        ctx_.player.stop();
        ctx_.player.setPlaylist(pl->getSongs(), pl->getName());
        ctx_.config.set("active_playlist", pl->getName());
        ctx_.config.save();
    }
}

ScreenId PlaylistListScreen::handleKey(const std::string& key) {
    const int count = static_cast<int>(ctx_.playlists.size());

    if (key == "0" || key == "q" || key == "Q" || key == "ESC") {
        return ScreenId::MainMenu;
    }
    if (count == 0) {
        return ScreenId::Stay;
    }
    if (key == "UP") {
        if (selected_ > 0) selected_--;
        return ScreenId::Stay;
    }
    if (key == "DOWN") {
        if (selected_ < count - 1) selected_++;
        return ScreenId::Stay;
    }
    if (key == "ENTER") {
        activate(selected_);
        return ScreenId::Stay;
    }
    // Quick pick by number (1..9).
    if (key.size() == 1 && key[0] >= '1' && key[0] <= '9') {
        const int index = key[0] - '1';
        if (index < count) {
            activate(index);
        }
    }
    return ScreenId::Stay;
}
