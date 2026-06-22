#pragma once

#include <string>
#include <vector>

#include "AppContext.h"
#include "Screen.h"

class Song;

// The "Browse" screen for the active playlist. Supports, as display-only
// transformations: sorting, case-insensitive search, and artist/album filtering
// (the latter is chosen on the dedicated Filter screen and applied here).
//
// Navigation is key-by-key: Up/Down move the highlight, Enter plays it.
class PlaylistViewScreen : public Screen {
public:
    explicit PlaylistViewScreen(AppContext& ctx) : Screen(ctx) {}

    void onEnter() override;
    void render() override;
    ScreenId handleKey(const std::string& key) override;

private:
    enum class SortField { Title, Artist, Album, Year, Duration };

    // Builds the list to display by applying filter/search, then sorting.
    std::vector<Song*> buildDisplayList() const;
    void cycleSort();
    void runSearchPrompt();
    ScreenId playSelection(const std::vector<Song*>& display, int displayIndex);
    std::string sortLabel() const;
    void clampSelection(int count);

    SortField sortField_ = SortField::Title;
    bool sortDescending_ = false;
    std::string searchQuery_;
    int selected_ = 0; // highlighted row in the current display list

    bool filterActive_ = false;
    FilterType filterType_ = FilterType::None;
    std::string filterValue_;
};
