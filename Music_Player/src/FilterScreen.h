#pragma once

#include <string>
#include <vector>

#include "AppContext.h"
#include "Screen.h"

// Two-stage filter chooser for the active playlist:
//   Stage 1: pick the field (Artist or Album).
//   Stage 2: pick one value from the distinct values present (with counts).
// On selection it stores a PendingFilter on the context and returns to the
// Playlist view, which applies it.
class FilterScreen : public Screen {
public:
    explicit FilterScreen(AppContext& ctx) : Screen(ctx) {}

    void onEnter() override;
    void render() override;
    ScreenId handleKey(const std::string& key) override;

private:
    enum class Stage { ChooseField, ChooseValue };

    struct ValueCount {
        std::string value;
        int count = 0;
    };

    void buildValues();

    Stage stage_ = Stage::ChooseField;
    FilterType chosenType_ = FilterType::None;
    std::vector<ValueCount> values_;
};
