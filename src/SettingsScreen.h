#pragma once

#include <string>

#include "Screen.h"

class SettingsScreen : public Screen {
public:
    explicit SettingsScreen(AppContext& ctx) : Screen(ctx) {}
    void render() override;
    ScreenId handleKey(const std::string& key) override;
};
