#include "Application.h"

#include <iostream>
#include <string>

#include "CsvLoader.h"
#include "FilterScreen.h"
#include "M3uLoader.h"
#include "MainMenuScreen.h"
#include "NowPlayingScreen.h"
#include "Playlist.h"
#include "PlaylistListScreen.h"
#include "PlaylistViewScreen.h"
#include "SettingsScreen.h"
#include "Song.h"

namespace {
const char* kLibraryCsv = "Data/library.csv";
const char* kPlaylistsDir = "Data/Playlists";
const char* kSettingsCfg = "Data/settings.cfg";

// How long pollKey waits each loop. Live screens use a short window so the timer
// stays smooth; other screens wait longer (they only redraw on a key press).
const int kLiveRefreshMs = 250;
const int kIdleRefreshMs = 1000;
} // namespace

Application::Application() {
    ctx_ = std::make_unique<AppContext>(library_, playlists_, player_, config_, ui_, input_);
}

Application::~Application() {
    // Defensive: ensure playlists are freed even if shutdown() was not called.
    for (Playlist* pl : playlists_) {
        delete pl;
    }
    playlists_.clear();
}

void Application::init() {
    // --- Load the song catalogue (I/O wrapped in try/catch) ----------------
    try {
        CsvLoader csv;
        const int loaded = csv.load(kLibraryCsv, library_);
        std::cout << "Loaded " << loaded << " songs from library.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error loading library: " << e.what() << "\n";
        std::cerr << "Continuing with an empty library.\n";
        dataDirReady_ = false;
    }

    // --- Load playlists -----------------------------------------------------
    M3uLoader m3u;
    playlists_ = m3u.loadAll(kPlaylistsDir, library_);
    std::cout << "Loaded " << playlists_.size() << " playlist(s).\n";

    // --- Load settings ------------------------------------------------------
    config_.load(kSettingsCfg);

    buildScreens();
    restoreState();
}

void Application::buildScreens() {
    screens_[ScreenId::MainMenu] = std::make_unique<MainMenuScreen>(*ctx_);
    screens_[ScreenId::NowPlaying] = std::make_unique<NowPlayingScreen>(*ctx_);
    screens_[ScreenId::PlaylistList] = std::make_unique<PlaylistListScreen>(*ctx_);
    screens_[ScreenId::PlaylistView] = std::make_unique<PlaylistViewScreen>(*ctx_);
    screens_[ScreenId::Filter] = std::make_unique<FilterScreen>(*ctx_);
    screens_[ScreenId::Settings] = std::make_unique<SettingsScreen>(*ctx_);
}

void Application::restoreState() {
    // Playback mode.
    if (config_.has("playback_mode")) {
        player_.setMode(Player::modeFromString(config_.get("playback_mode")));
    }

    // Active playlist: match by saved name, else default to the first one.
    int activeIndex = playlists_.empty() ? -1 : 0;
    const std::string savedName = config_.get("active_playlist");
    if (!savedName.empty()) {
        for (std::size_t i = 0; i < playlists_.size(); ++i) {
            if (playlists_[i]->getName() == savedName) {
                activeIndex = static_cast<int>(i);
                break;
            }
        }
    }
    ctx_->activePlaylistIndex = activeIndex;

    if (activeIndex >= 0) {
        Playlist* pl = playlists_[static_cast<std::size_t>(activeIndex)];
        player_.setPlaylist(pl->getSongs(), pl->getName());
    }

    // Last played song (shown but not auto-played).
    const std::string lastSong = config_.get("last_song");
    if (!lastSong.empty()) {
        player_.selectByPath(lastSong);
    }
}

void Application::run() {
    ui_.initConsole();

    ScreenId current = ScreenId::MainMenu;
    screens_[current]->onEnter();
    bool needRender = true;

    while (true) {
        // Auto-advance if the current song finished.
        player_.tick();

        Screen* screen = screens_[current].get();
        if (needRender) {
            ui_.beginFrame();
            screen->render();
            ui_.present();
            needRender = false;
        }

        const bool live = screen->isLive();
        const std::string key = input_.pollKey(live ? kLiveRefreshMs : kIdleRefreshMs);
        if (key.empty()) {
            // Timed out with no key: only live screens need a fresh frame so the
            // elapsed time keeps moving. Other screens stay perfectly still.
            if (live) {
                needRender = true;
            }
            continue;
        }

        const ScreenId next = screen->handleKey(key);
        if (next == ScreenId::Quit) {
            break;
        }
        if (next != ScreenId::Stay && next != current) {
            current = next;
            screens_[current]->onEnter();
        }
        needRender = true; // a key was handled -> redraw
    }

    ui_.shutdownConsole();
}

void Application::shutdown() {
    // Persist current state.
    Playlist* active = ctx_->activePlaylist();
    if (active != nullptr) {
        config_.set("active_playlist", active->getName());
    }
    config_.set("playback_mode", Player::modeToString(player_.getMode()));
    Song* current = player_.getCurrentSong();
    if (current != nullptr) {
        config_.set("last_song", current->getFilePath());
    }
    config_.save();

    // Free the playlists we own (songs are freed by MusicLibrary).
    for (Playlist* pl : playlists_) {
        delete pl;
    }
    playlists_.clear();

    std::cout << "Goodbye!\n";
}
