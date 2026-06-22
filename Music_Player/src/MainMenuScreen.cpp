#include "MainMenuScreen.h"

#include "AppContext.h"
#include "ConfigManager.h"
#include "InputHandler.h"
#include "MusicLibrary.h"
#include "Player.h"
#include "Song.h"
#include "UIRenderer.h"

void MainMenuScreen::render() {
    UIRenderer& ui = ctx_.ui;
    ui.header("MUSIC PLAYER");

    // "Last played" line, resolved from the saved last_song path.
    const std::string lastPath = ctx_.config.get("last_song");
    if (!lastPath.empty()) {
        Song* song = ctx_.library.getSong(lastPath);
        if (song != nullptr) {
            ui.boxLine("Last played: " + song->getTitle() + " - " + song->getArtist());
        } else {
            ui.boxLine("Last played: (unavailable)");
        }
    } else {
        ui.boxLine("Last played: (none yet)");
    }
    ui.boxLine("Mode: " + Player::modeToString(ctx_.player.getMode()));
    ui.boxSeparator();

    ui.boxLine("[1] Now Playing");
    ui.boxLine("[2] Playlists");
    ui.boxLine("[3] Browse Active Playlist");
    ui.boxLine("[4] Settings");
    ui.boxLine("[0] Quit");
    ui.boxSeparator();
    ui.boxLine("Press a number key (no Enter needed).");
    ui.boxBottom();
}

ScreenId MainMenuScreen::handleKey(const std::string& key) {
    if (key == "1") return ScreenId::NowPlaying;
    if (key == "2") return ScreenId::PlaylistList;
    if (key == "3") {
        ctx_.pendingFilter = PendingFilter{}; // browse with no active filter
        return ScreenId::PlaylistView;
    }
    if (key == "4") return ScreenId::Settings;
    if (key == "0" || key == "q" || key == "ESC") return ScreenId::Quit;
    return ScreenId::Stay;
}
