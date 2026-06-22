#include "SettingsScreen.h"

#include <string>

#include "AppContext.h"
#include "ConfigManager.h"
#include "InputHandler.h"
#include "Player.h"
#include "UIRenderer.h"

namespace {
std::string marker(PlaybackMode current, PlaybackMode option) {
    return current == option ? "   * current" : "";
}
} // namespace

void SettingsScreen::render() {
    UIRenderer& ui = ctx_.ui;
    const PlaybackMode mode = ctx_.player.getMode();
    ui.header("SETTINGS - PLAYBACK MODE");
    ui.boxLine("[1] No Repeat" + marker(mode, PlaybackMode::NO_REPEAT));
    ui.boxLine("[2] Repeat One" + marker(mode, PlaybackMode::REPEAT_ONE));
    ui.boxLine("[3] Repeat All" + marker(mode, PlaybackMode::REPEAT_ALL));
    ui.boxLine("[4] Shuffle" + marker(mode, PlaybackMode::SHUFFLE));
    ui.boxSeparator();
    ui.boxLine("Press 1-4 to change (saved automatically).");
    ui.boxLine("[0] Back");
    ui.boxBottom();
}

ScreenId SettingsScreen::handleKey(const std::string& key) {
    if (key == "0" || key == "q" || key == "Q" || key == "ESC") {
        return ScreenId::MainMenu;
    }

    PlaybackMode mode = ctx_.player.getMode();
    bool changed = true;
    if (key == "1") {
        mode = PlaybackMode::NO_REPEAT;
    } else if (key == "2") {
        mode = PlaybackMode::REPEAT_ONE;
    } else if (key == "3") {
        mode = PlaybackMode::REPEAT_ALL;
    } else if (key == "4") {
        mode = PlaybackMode::SHUFFLE;
    } else {
        changed = false;
    }

    if (changed) {
        ctx_.player.setMode(mode);
        ctx_.config.set("playback_mode", Player::modeToString(mode));
        ctx_.config.save();
    }
    return ScreenId::Stay;
}
