#include "NowPlayingScreen.h"

#include <string>

#include "AppContext.h"
#include "InputHandler.h"
#include "Player.h"
#include "Song.h"
#include "UIRenderer.h"

std::string NowPlayingScreen::progressBar() const {
    const int width = 40;
    const int total = ctx_.player.getDurationSec();
    const int cur = ctx_.player.getCursorSec();
    int filled = 0;
    if (total > 0) {
        filled = (cur * width) / total;
        if (filled > width) filled = width;
        if (filled < 0) filled = 0;
    }
    // ASCII bar keeps the byte width predictable inside the box.
    return "[" + std::string(static_cast<std::size_t>(filled), '#') +
           std::string(static_cast<std::size_t>(width - filled), '-') + "]";
}

void NowPlayingScreen::render() {
    UIRenderer& ui = ctx_.ui;
    ui.header("NOW PLAYING");

    Song* song = ctx_.player.getCurrentSong();
    if (song == nullptr) {
        ui.boxLine("No song selected.");
        ui.boxLine("Open a playlist to pick something to play.");
    } else {
        ui.boxLine("Title : " + song->getTitle());
        ui.boxLine("Artist: " + song->getArtist());
        ui.boxLine("Album : " + song->getAlbum());
        ui.boxLine("State : " + Player::stateToString(ctx_.player.getState()));
        ui.boxLine("");
        ui.boxLine(progressBar());
        ui.boxLine("Time  : " + ctx_.player.getTimeString());
    }
    ui.boxLine("Mode  : " + Player::modeToString(ctx_.player.getMode()));
    ui.boxLine("List  : " + ctx_.player.getPlaylistName());

    if (!ctx_.player.isAudioAvailable()) {
        ui.boxSeparator();
        ui.boxLine("(!) No audio device - timer is simulated.");
    }

    ui.boxSeparator();
    ui.boxLine("[p] Play/Pause   [s] Stop");
    ui.boxLine("[n] Next         [b] Previous");
    ui.boxLine("[<-/-] Back 10s  [->/+] Forward 10s");
    ui.boxLine("[0] Back to menu");
    ui.boxBottom();
}

ScreenId NowPlayingScreen::handleKey(const std::string& key) {
    if (key == "p" || key == "P" || key == "ENTER") {
        if (ctx_.player.getState() == PlayerState::PLAYING) {
            ctx_.player.pause();
        } else if (ctx_.player.getState() == PlayerState::PAUSED) {
            ctx_.player.resume();
        } else {
            ctx_.player.play();
        }
    } else if (key == "n" || key == "N") {
        ctx_.player.next();
    } else if (key == "b" || key == "B") {
        ctx_.player.previous();
    } else if (key == "s" || key == "S") {
        ctx_.player.stop();
    } else if (key == "+" || key == "=" || key == "RIGHT") {
        ctx_.player.seekBy(10);
    } else if (key == "-" || key == "_" || key == "LEFT") {
        ctx_.player.seekBy(-10);
    } else if (key == "0" || key == "q" || key == "Q" || key == "ESC") {
        return ScreenId::MainMenu;
    }
    return ScreenId::Stay;
}
