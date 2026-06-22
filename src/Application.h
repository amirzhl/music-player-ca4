#pragma once

#include <map>
#include <memory>
#include <vector>

#include "AppContext.h"
#include "ConfigManager.h"
#include "InputHandler.h"
#include "MusicLibrary.h"
#include "Player.h"
#include "Screen.h"
#include "UIRenderer.h"

class Playlist;

// Top-level coordinator: owns every subsystem, wires them together via
// AppContext, drives the render/input loop and persists state on exit.
//
// Ownership summary:
//   - library_  owns all Song objects.
//   - playlists_ are owned here (deleted in shutdown); they do NOT own songs.
//   - screens_  are owned here via unique_ptr.
class Application {
public:
    Application();
    ~Application();

    void init();     // load data + restore saved state
    void run();      // main loop
    void shutdown(); // persist state + free playlists

private:
    void buildScreens();
    void restoreState();

    MusicLibrary library_;
    std::vector<Playlist*> playlists_;
    ConfigManager config_;
    UIRenderer ui_;
    InputHandler input_;
    Player player_;

    std::unique_ptr<AppContext> ctx_;
    std::map<ScreenId, std::unique_ptr<Screen>> screens_;

    bool dataDirReady_ = true;
};
