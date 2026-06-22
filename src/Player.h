#pragma once

#include <string>
#include <vector>

#include "miniaudio.h"

class Song;

enum class PlayerState { STOPPED, PLAYING, PAUSED };
enum class PlaybackMode { NO_REPEAT, REPEAT_ONE, REPEAT_ALL, SHUFFLE };

// The playback state machine. Wraps the miniaudio engine (the only place audio
// is touched) and exposes a clean, UI-independent control surface.
//
// Dynamic memory / RAII: the miniaudio engine is initialised in the constructor
// and released in the destructor; the destructor is the matching free path for
// every sound resource loaded during playback.
class Player {
public:
    Player();
    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // Replace the active set of songs (e.g. when switching playlist). Stops any
    // current playback and clears the current selection.
    void setPlaylist(const std::vector<Song*>& songs, const std::string& name);
    const std::string& getPlaylistName() const { return playlistName_; }

    // Playback controls.
    bool playIndex(int index); // start the given song from the beginning
    void play();               // (re)start the current/first song
    void pause();              // keep position
    void resume();             // continue from paused position
    void stop();               // reset position to zero
    void next();               // advance according to the active mode
    void previous();           // go to the previous song

    // Called once per main-loop iteration: auto-advances when a song ends.
    void tick();

    void setMode(PlaybackMode mode) { mode_ = mode; }
    PlaybackMode getMode() const { return mode_; }
    PlayerState getState() const { return state_; }

    // True if the audio engine initialised, i.e. a usable audio device exists.
    // When false, playback is silent but every other feature keeps working.
    bool isAudioAvailable() const { return engineReady_; }

    Song* getCurrentSong() const { return currentSong_; }
    bool hasSongs() const { return !songs_.empty(); }

    // Used at startup to show the "last played" song without starting playback.
    void selectByPath(const std::string& filePath);

    // Time helpers (current cursor / total length).
    float getCursorSec() const;
    int getDurationSec() const;
    std::string getTimeString() const; // "MM:SS / MM:SS"

    // Optional bonus: relative seek (+/- seconds) for arrow-key support.
    void seekBy(int seconds);

    // One-shot status message (e.g. "Audio file not found"). Reading clears it.
    std::string consumeMessage();

    static std::string modeToString(PlaybackMode mode);
    static PlaybackMode modeFromString(const std::string& text);
    static std::string stateToString(PlayerState state);

private:
    // Tries to start playback at `index`, skipping songs whose files are
    // missing. Returns true if a song actually started.
    bool startAt(int index);
    void loadAndStart(Song* song);
    void unloadSound();
    int pickNextIndex() const;
    int pickPrevIndex() const;
    bool fileExists(const std::string& path) const;

    std::vector<Song*> songs_;   // non-owning; owned by MusicLibrary
    std::string playlistName_;
    int currentIndex_ = -1;
    Song* currentSong_ = nullptr;

    PlayerState state_ = PlayerState::STOPPED;
    PlaybackMode mode_ = PlaybackMode::NO_REPEAT;

    std::string message_;

    // miniaudio resources.
    ma_engine engine_{};
    ma_sound sound_{};
    bool engineReady_ = false;
    bool soundLoaded_ = false;

    // Resume support: stop() remembers the cursor (in PCM frames) so the next
    // play() continues from where it was stopped instead of restarting.
    ma_uint64 resumeFrame_ = 0;
    bool resumePending_ = false;
};
