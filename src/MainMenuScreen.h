#pragma once

#include <string>

#include "Screen.h"

class MainMenuScreen : public Screen {
public:
    explicit MainMenuScreen(AppContext& ctx) : Screen(ctx) {}
    void render() override;
    ScreenId handleKey(const std::string& key) override;
};
