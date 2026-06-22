#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "miniaudio.h"

class Song;

enum class PlayerState { STOPPED, PLAYING, PAUSED };
enum class PlaybackMode { NO_REPEAT, REPEAT_ONE, REPEAT_ALL, SHUFFLE };

// The playback state machine.
//
// Audio architecture (rationale): a SINGLE playback device (ma_device) is
// created once in the constructor and kept running for the whole lifetime of
// the player. It is never re-created or torn down on a track change. Each song
// is opened as a passive ma_decoder (a data source). Changing track only swaps
// the *decoder* behind a short-held mutex; the always-running audio device
// keeps pulling frames through a single callback.
//
// Why: the previous design used the miniaudio high-level engine and called
// ma_sound_init/uninit on every track change. Destroying a sound while the
// device thread was actively mixing it could deadlock inside the audio backend
// on Windows -- the symptom was the UI freezing on "next" while audio kept
// playing, leaving orphaned music_player.exe processes behind. By never
// destroying the device during playback and only swapping a passive decoder
// pointer under a brief lock, that whole class of teardown deadlocks is gone.
//
// Threading: every public method runs on the main thread. dataCallback() runs
// on the audio thread. They share decoder_/playing_/atEnd_ and synchronise via
// audioMutex_ (held only for very short, bounded sections -- never across a
// blocking call), so there is no lock-ordering deadlock.
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

    // True if the audio device initialised, i.e. a usable audio device exists.
    // When false, playback is silent but every other feature keeps working.
    bool isAudioAvailable() const { return deviceReady_; }

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
    void unloadDecoder();

    int pickNextIndex() const;
    int pickPrevIndex() const;
    bool fileExists(const std::string& path) const;

    // Audio thread entry points.
    static void dataCallback(ma_device* device, void* output, const void* input,
                             ma_uint32 frameCount);
    void mixAudio(void* output, ma_uint32 frameCount);

    std::vector<Song*> songs_;   // non-owning; owned by MusicLibrary
    std::string playlistName_;
    int currentIndex_ = -1;
    Song* currentSong_ = nullptr;

    PlayerState state_ = PlayerState::STOPPED;
    PlaybackMode mode_ = PlaybackMode::NO_REPEAT;

    std::string message_;

    // miniaudio resources.
    // The device is created once and lives for the whole program. The decoder
    // is heap-allocated and swapped on every track change (pointer swap only,
    // so the audio thread never sees a half-initialised decoder).
    ma_device device_{};
    ma_decoder* decoder_ = nullptr;     // guarded by audioMutex_
    bool deviceReady_ = false;
    ma_uint32 channels_ = 2;
    ma_uint32 sampleRate_ = 48000;

    // Synchronises the main thread and the audio callback. mutable so the const
    // time-query helpers can lock it.
    mutable std::mutex audioMutex_;
    std::atomic<bool> playing_{false}; // should the callback pull frames?
    std::atomic<bool> atEnd_{false};   // set by the callback when a song ends

    // Lock-free playback position/length, published by the audio thread (and by
    // seek/stop on the main thread) and read by the UI. Keeping the per-frame
    // time display out of audioMutex_ means the UI never blocks the audio
    // callback -- which is what made heavy (high-bitrate/VBR) files stutter.
    std::atomic<ma_uint64> cursorFrames_{0};
    std::atomic<ma_uint64> lengthFrames_{0};
};
