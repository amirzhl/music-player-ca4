#pragma once

#include <string>
#include <utility>
#include <vector>

// Low-level terminal renderer.
//
// Drawing is double-buffered: a screen builds up rows with the box* helpers and
// then present() paints the whole frame at once. Instead of clearing the screen
// (which causes flicker), present() moves the cursor home and overwrites every
// line, clearing only the leftovers at the end. This gives a smooth, flicker
// free redraw -- important for the live timer on the Now Playing screen.
class UIRenderer {
public:
    static constexpr int kWidth = 56; // inner width between the side borders

    // One-time terminal setup/teardown (UTF-8, ANSI colours, hidden cursor).
    void initConsole();
    void shutdownConsole();

    // Frame lifecycle.
    void beginFrame();   // start a new (empty) frame
    void present();      // paint the frame without flicker
    void fullClear();    // hard clear (used before modal text input)
    void showCursor();   // make the caret visible (for typing)
    void hideCursor();   // hide the caret again

    // Frame building blocks (append rows to the current frame).
    void boxTop();
    void boxSeparator();
    void boxBottom();
    void boxLine(const std::string& text = ""); // left-aligned content row
    void boxSelected(const std::string& text);  // highlighted (selected) row
    void boxCentered(const std::string& text);  // centred content row
    void header(const std::string& title);      // top border + title + divider

private:
    std::string fitLeft(const std::string& text) const;
    std::string fitCentered(const std::string& text) const;
    void push(const std::string& line, bool highlighted = false);

    // Each row: the fully-built text (with borders) and whether to highlight it.
    std::vector<std::pair<std::string, bool>> rows_;
    bool ansiEnabled_ = true; // false -> fall back to a plain clear+redraw
};
