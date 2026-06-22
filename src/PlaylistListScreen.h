#pragma once

#include <string>

#include "Screen.h"

class PlaylistListScreen : public Screen {
public:
    explicit PlaylistListScreen(AppContext& ctx) : Screen(ctx) {}
    void onEnter() override;
    void render() override;
    ScreenId handleKey(const std::string& key) override;

private:
    void activate(int index);
    int selected_ = 0; // highlighted row
};
