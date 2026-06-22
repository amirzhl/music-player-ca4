#pragma once

#include <string>

#include "Screen.h"

class NowPlayingScreen : public Screen {
public:
    explicit NowPlayingScreen(AppContext& ctx) : Screen(ctx) {}
    void render() override;
    ScreenId handleKey(const std::string& key) override;
    bool isLive() const override { return true; } // keep the timer ticking

private:
    std::string progressBar() const;
};
