#include "FilterScreen.h"

#include <algorithm>
#include <string>

#include "InputHandler.h"
#include "Playlist.h"
#include "Song.h"
#include "UIRenderer.h"
#include "Utils.h"

void FilterScreen::onEnter() {
    stage_ = Stage::ChooseField;
    chosenType_ = FilterType::None;
    values_.clear();
}

void FilterScreen::buildValues() {
    values_.clear();
    Playlist* pl = ctx_.activePlaylist();
    if (pl == nullptr) {
        return;
    }
    for (Song* s : pl->getSongs()) {
        if (s == nullptr) {
            continue;
        }
        const std::string key =
            (chosenType_ == FilterType::Artist) ? s->getArtist() : s->getAlbum();
        auto it = std::find_if(values_.begin(), values_.end(),
                               [&key](const ValueCount& vc) {
                                   return utils::iequals(vc.value, key);
                               });
        if (it == values_.end()) {
            values_.push_back({key, 1});
        } else {
            it->count++;
        }
    }
    std::sort(values_.begin(), values_.end(),
              [](const ValueCount& a, const ValueCount& b) {
                  return utils::toLower(a.value) < utils::toLower(b.value);
              });
}

void FilterScreen::render() {
    UIRenderer& ui = ctx_.ui;
    if (stage_ == Stage::ChooseField) {
        ui.header("FILTER BY");
        ui.boxLine("[1] Artist");
        ui.boxLine("[2] Album");
        ui.boxSeparator();
        ui.boxLine("[0] Back");
        ui.boxBottom();
    } else {
        const std::string kind = (chosenType_ == FilterType::Artist) ? "ARTIST" : "ALBUM";
        ui.header("FILTER BY " + kind);
        if (values_.empty()) {
            ui.boxLine("(no values available)");
        } else {
            for (std::size_t i = 0; i < values_.size(); ++i) {
                ui.boxLine("[" + std::to_string(i + 1) + "] " + values_[i].value +
                           "  (" + std::to_string(values_[i].count) + ")");
            }
        }
        ui.boxSeparator();
        ui.boxLine("Press a number to apply.   [0] Back");
        ui.boxBottom();
    }
}

ScreenId FilterScreen::handleKey(const std::string& key) {
    if (stage_ == Stage::ChooseField) {
        if (key == "0" || key == "q" || key == "Q" || key == "ESC") {
            return ScreenId::PlaylistView; // back, no filter applied
        }
        if (key == "1" || key == "2") {
            chosenType_ = (key == "1") ? FilterType::Artist : FilterType::Album;
            buildValues();
            stage_ = Stage::ChooseValue;
        }
        return ScreenId::Stay;
    }

    // Stage::ChooseValue
    if (key == "0" || key == "q" || key == "Q" || key == "ESC" || key == "BACK") {
        stage_ = Stage::ChooseField; // go back to field selection
        return ScreenId::Stay;
    }

    const int count = static_cast<int>(values_.size());
    if (key.size() == 1 && key[0] >= '1' && key[0] <= '9') {
        const int index = key[0] - '1';
        if (index < count) {
            ctx_.pendingFilter.type = chosenType_;
            ctx_.pendingFilter.value = values_[static_cast<std::size_t>(index)].value;
            return ScreenId::PlaylistView; // applied; Playlist view consumes it
        }
    }
    return ScreenId::Stay;
}
