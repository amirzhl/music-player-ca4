#include "PlaylistViewScreen.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "InputHandler.h"
#include "MusicLibrary.h"
#include "Player.h"
#include "Playlist.h"
#include "Song.h"
#include "UIRenderer.h"
#include "Utils.h"

void PlaylistViewScreen::onEnter() {
    // Reset display-only state on every fresh entry.
    sortField_ = SortField::Title;
    sortDescending_ = false;
    searchQuery_.clear();
    selected_ = 0;

    // Consume any pending filter handed over by the Filter screen.
    if (ctx_.pendingFilter.type != FilterType::None) {
        filterActive_ = true;
        filterType_ = ctx_.pendingFilter.type;
        filterValue_ = ctx_.pendingFilter.value;
    } else {
        filterActive_ = false;
        filterType_ = FilterType::None;
        filterValue_.clear();
    }
    ctx_.pendingFilter = PendingFilter{}; // consumed
}

std::vector<Song*> PlaylistViewScreen::buildDisplayList() const {
    Playlist* pl = ctx_.activePlaylist();
    std::vector<Song*> songs;
    if (pl != nullptr) {
        songs = pl->getSongs();
    }

    if (filterActive_) {
        if (filterType_ == FilterType::Artist) {
            songs = MusicLibrary::filterByArtist(songs, filterValue_);
        } else if (filterType_ == FilterType::Album) {
            songs = MusicLibrary::filterByAlbum(songs, filterValue_);
        }
    } else if (!searchQuery_.empty()) {
        songs = MusicLibrary::search(songs, searchQuery_);
    }

    const SortField field = sortField_;
    const bool desc = sortDescending_;
    std::stable_sort(songs.begin(), songs.end(), [field, desc](Song* a, Song* b) {
        if (a == nullptr || b == nullptr) {
            return false;
        }
        bool less;
        switch (field) {
            case SortField::Artist:
                less = utils::toLower(a->getArtist()) < utils::toLower(b->getArtist());
                break;
            case SortField::Album:
                less = utils::toLower(a->getAlbum()) < utils::toLower(b->getAlbum());
                break;
            case SortField::Year:
                less = a->getYear() < b->getYear();
                break;
            case SortField::Duration:
                less = a->getDuration() < b->getDuration();
                break;
            case SortField::Title:
            default:
                less = utils::toLower(a->getTitle()) < utils::toLower(b->getTitle());
                break;
        }
        return desc ? !less : less;
    });
    return songs;
}

std::string PlaylistViewScreen::sortLabel() const {
    std::string field;
    switch (sortField_) {
        case SortField::Title: field = "Title"; break;
        case SortField::Artist: field = "Artist"; break;
        case SortField::Album: field = "Album"; break;
        case SortField::Year: field = "Year"; break;
        case SortField::Duration: field = "Duration"; break;
    }
    return field + (sortDescending_ ? " v" : " ^");
}

void PlaylistViewScreen::clampSelection(int count) {
    if (count <= 0) {
        selected_ = 0;
        return;
    }
    if (selected_ < 0) selected_ = 0;
    if (selected_ >= count) selected_ = count - 1;
}

void PlaylistViewScreen::render() {
    UIRenderer& ui = ctx_.ui;
    Playlist* pl = ctx_.activePlaylist();
    const std::string name = (pl != nullptr) ? pl->getName() : "(no playlist)";

    ui.header("BROWSE: " + name);

    // Status line describing the current display transformation.
    if (filterActive_) {
        const std::string kind = (filterType_ == FilterType::Artist) ? "Artist" : "Album";
        ui.boxLine("Filter: " + kind + " = " + filterValue_);
    } else if (!searchQuery_.empty()) {
        ui.boxLine("Search: \"" + searchQuery_ + "\"");
    } else {
        ui.boxLine("Sort: " + sortLabel());
    }
    ui.boxSeparator();

    const std::vector<Song*> display = buildDisplayList();
    if (display.empty()) {
        ui.boxLine("(no songs to show)");
    } else {
        for (std::size_t i = 0; i < display.size(); ++i) {
            Song* s = display[i];
            std::ostringstream row;
            row << std::setw(2) << (i + 1) << ". ";
            std::string title = s->getTitle();
            std::string artist = s->getArtist();
            if (title.size() > 26) title = title.substr(0, 25) + ".";
            if (artist.size() > 16) artist = artist.substr(0, 15) + ".";
            row << std::left << std::setw(27) << title
                << std::setw(17) << artist
                << s->getDurationString();
            if (static_cast<int>(i) == selected_) {
                ui.boxSelected(row.str());
            } else {
                ui.boxLine(row.str());
            }
        }
    }

    ui.boxSeparator();
    if (filterActive_) {
        ui.boxLine("[Up/Down] move  [Enter] play  [f] new filter");
        ui.boxLine("[0] clear filter");
    } else if (!searchQuery_.empty()) {
        ui.boxLine("[Up/Down] move  [Enter] play  [/] new search");
        ui.boxLine("[0] clear search");
    } else {
        ui.boxLine("[Up/Down] move  [Enter] play  [s] sort  [d] dir");
        ui.boxLine("[f] filter   [/] search   [0] back");
    }
    ui.boxBottom();
}

void PlaylistViewScreen::cycleSort() {
    switch (sortField_) {
        case SortField::Title: sortField_ = SortField::Artist; break;
        case SortField::Artist: sortField_ = SortField::Album; break;
        case SortField::Album: sortField_ = SortField::Year; break;
        case SortField::Year: sortField_ = SortField::Duration; break;
        case SortField::Duration: sortField_ = SortField::Title; break;
    }
}

void PlaylistViewScreen::runSearchPrompt() {
    // A simple modal text-entry: clear the screen, show the cursor, read a line.
    ctx_.ui.showCursor();
    ctx_.ui.fullClear();
    std::cout << "  SEARCH\n";
    std::cout << "  Type a query (title / artist / album).\n";
    std::cout << "  Leave empty and press Enter to clear the search.\n\n";
    searchQuery_ = ctx_.input.readLine("  > ");
    ctx_.ui.hideCursor();
    selected_ = 0;
}

ScreenId PlaylistViewScreen::playSelection(const std::vector<Song*>& display, int displayIndex) {
    if (displayIndex < 1 || displayIndex > static_cast<int>(display.size())) {
        return ScreenId::Stay;
    }
    Song* chosen = display[static_cast<std::size_t>(displayIndex - 1)];

    // Map the chosen song back to its index in the full active playlist so that
    // next/previous operate over the whole playlist.
    Playlist* pl = ctx_.activePlaylist();
    if (pl == nullptr) {
        return ScreenId::Stay;
    }
    const std::vector<Song*>& full = pl->getSongs();
    int globalIndex = 0;
    for (std::size_t i = 0; i < full.size(); ++i) {
        if (full[i] == chosen) {
            globalIndex = static_cast<int>(i);
            break;
        }
    }
    ctx_.player.playIndex(globalIndex);
    return ScreenId::NowPlaying;
}

ScreenId PlaylistViewScreen::handleKey(const std::string& key) {
    std::vector<Song*> display = buildDisplayList();
    const int count = static_cast<int>(display.size());
    clampSelection(count);

    // Back / clear semantics depend on the current mode.
    if (key == "0" || key == "q" || key == "Q" || key == "ESC") {
        if (filterActive_) {
            filterActive_ = false;
            filterType_ = FilterType::None;
            filterValue_.clear();
            selected_ = 0;
            return ScreenId::Stay;
        }
        if (!searchQuery_.empty()) {
            searchQuery_.clear();
            selected_ = 0;
            return ScreenId::Stay;
        }
        return ScreenId::MainMenu;
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
        if (count > 0) {
            return playSelection(display, selected_ + 1);
        }
        return ScreenId::Stay;
    }

    // Quick play by number (1..9).
    if (key.size() == 1 && key[0] >= '1' && key[0] <= '9') {
        const int index = key[0] - '0';
        if (index <= count) {
            return playSelection(display, index);
        }
        return ScreenId::Stay;
    }

    if ((key == "s" || key == "S") && !filterActive_) {
        cycleSort();
        selected_ = 0;
    } else if ((key == "d" || key == "D") && !filterActive_) {
        sortDescending_ = !sortDescending_;
        selected_ = 0;
    } else if (key == "f" || key == "F") {
        return ScreenId::Filter;
    } else if (key == "/") {
        runSearchPrompt();
    }
    return ScreenId::Stay;
}
