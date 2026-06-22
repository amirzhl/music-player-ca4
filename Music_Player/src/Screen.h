#pragma once

#include <string>

class AppContext;

// Identifies every screen, plus two control values:
//  - Stay: keep the current screen (re-render without re-entering it).
//  - Quit: leave the application (only ever returned from the Main Menu).
enum class ScreenId {
    MainMenu,
    NowPlaying,
    PlaylistList,
    PlaylistView,
    Filter,
    Settings,
    Stay,
    Quit
};

// Abstract base class for all screens.
//
// Demonstrates the four OOP pillars used throughout the UI layer:
//  - Abstraction: render() is pure virtual.
//  - Polymorphism: the Application drives screens through Screen* pointers and a
//    virtual destructor guarantees correct cleanup.
//  - Inheritance: every concrete screen derives from Screen.
//  - Encapsulation: shared services are reached only via the protected ctx_.
class Screen {
public:
    explicit Screen(AppContext& ctx) : ctx_(ctx) {}
    virtual ~Screen() = default;

    // Called once when the screen becomes active (resets transient state).
    virtual void onEnter() {}

    // Draw the screen into the renderer's frame buffer. Pure virtual, so Screen
    // itself cannot be instantiated.
    virtual void render() = 0;

    // React to a single key press (see InputHandler::pollKey for the token
    // format, e.g. "a", "1", "UP", "ENTER", "ESC"). Returns the next screen to
    // show; ScreenId::Stay keeps the user in place.
    virtual ScreenId handleKey(const std::string& key) = 0;

    // "Live" screens (only Now Playing) keep redrawing on a timer so the
    // elapsed time advances smoothly even when no key is pressed.
    virtual bool isLive() const { return false; }

protected:
    AppContext& ctx_;
};
