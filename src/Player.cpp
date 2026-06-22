// The MINIAUDIO_IMPLEMENTATION macro must be defined in EXACTLY ONE .cpp file.
// Defining it here (and nowhere else) avoids "multiple definition" linker errors.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "Player.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>

#include "Song.h"
#include "Utils.h"

namespace {
// A single, lazily-seeded RNG for SHUFFLE. Local to this translation unit, so it
// is not a mutable global accessible across the program.
int randomIndex(int count, int exclude) {
    static std::mt19937 rng{std::random_device{}()};
    if (count <= 1) {
        return 0;
    }
    std::uniform_int_distribution<int> dist(0, count - 1);
    int idx = dist(rng);
    // Avoid immediately repeating the same song.
    while (idx == exclude) {
        idx = dist(rng);
    }
    return idx;
}
} // namespace

Player::Player() {
    engineReady_ = (ma_engine_init(nullptr, &engine_) == MA_SUCCESS);
    if (!engineReady_) {
        // No usable audio backend/device was found (typical on WSL, plain SSH
        // sessions, containers, or VMs without a sound device). The player
        // still runs as a complete state machine -- it just makes no sound.
        std::cerr << "[audio] could not initialise the audio engine -- "
                     "running in silent mode (no audio device detected).\n";
    }
}

Player::~Player() {
    unloadSound();
    if (engineReady_) {
        ma_engine_uninit(&engine_);
        engineReady_ = false;
    }
}

bool Player::fileExists(const std::string& path) const {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

void Player::setPlaylist(const std::vector<Song*>& songs, const std::string& name) {
    stop();
    songs_ = songs;
    playlistName_ = name;
    currentIndex_ = -1;
    currentSong_ = nullptr;
    resumePending_ = false;
}

void Player::selectByPath(const std::string& filePath) {
    for (std::size_t i = 0; i < songs_.size(); ++i) {
        if (songs_[i] != nullptr && songs_[i]->getFilePath() == filePath) {
            currentIndex_ = static_cast<int>(i);
            currentSong_ = songs_[i];
            state_ = PlayerState::STOPPED;
            resumePending_ = false;
            return;
        }
    }
}

void Player::unloadSound() {
    if (soundLoaded_) {
        ma_sound_uninit(&sound_);
        soundLoaded_ = false;
    }
}

void Player::loadAndStart(Song* song) {
    unloadSound();
    if (!engineReady_) {
        message_ = "Audio engine unavailable -- playing in silent mode.";
        return;
    }
    if (song == nullptr) {
        return;
    }
    const ma_result result = ma_sound_init_from_file(
        &engine_, song->getFilePath().c_str(), 0, nullptr, nullptr, &sound_);
    if (result != MA_SUCCESS) {
        soundLoaded_ = false;
        message_ = "Could not decode audio file: " + song->getFilePath();
        return;
    }
    soundLoaded_ = true;
    ma_sound_start(&sound_);
}

bool Player::startAt(int index) {
    const int n = static_cast<int>(songs_.size());
    if (n == 0) {
        message_ = "Playlist is empty.";
        state_ = PlayerState::STOPPED;
        currentIndex_ = -1;
        currentSong_ = nullptr;
        return false;
    }

    // Normalise and try up to n songs, skipping those with missing files.
    int idx = ((index % n) + n) % n;
    for (int attempt = 0; attempt < n; ++attempt) {
        Song* song = songs_[idx];
        if (song != nullptr && fileExists(song->getFilePath())) {
            currentIndex_ = idx;
            currentSong_ = song;
            loadAndStart(song);
            state_ = PlayerState::PLAYING;
            return true;
        }
        if (song != nullptr) {
            message_ = "Audio file not found, skipping: " + song->getFilePath();
        }
        idx = (idx + 1) % n;
    }

    message_ = "No playable audio files in this playlist.";
    state_ = PlayerState::STOPPED;
    return false;
}

bool Player::playIndex(int index) {
    resumePending_ = false; // an explicit pick always starts from the top
    return startAt(index);
}

void Player::play() {
    if (songs_.empty()) {
        message_ = "Playlist is empty.";
        return;
    }
    // Resume from where the song was stopped (if any) instead of restarting it.
    const int idx = currentIndex_ < 0 ? 0 : currentIndex_;
    const bool resume = resumePending_ && currentIndex_ >= 0;
    const ma_uint64 frame = resumeFrame_;
    resumePending_ = false;
    if (startAt(idx) && resume && soundLoaded_) {
        ma_sound_seek_to_pcm_frame(&sound_, frame);
    }
}

void Player::pause() {
    if (state_ != PlayerState::PLAYING) {
        return; // silently ignored when nothing is playing
    }
    if (soundLoaded_) {
        ma_sound_stop(&sound_);
    }
    state_ = PlayerState::PAUSED;
}

void Player::resume() {
    if (state_ != PlayerState::PAUSED) {
        return; // silently ignored
    }
    if (soundLoaded_) {
        ma_sound_start(&sound_);
    }
    state_ = PlayerState::PLAYING;
}

void Player::stop() {
    if (soundLoaded_) {
        // Remember the current position so a following play() resumes from here
        // rather than restarting the track from the beginning.
        ma_uint64 cursor = 0;
        ma_sound_get_cursor_in_pcm_frames(&sound_, &cursor);
        resumeFrame_ = cursor;
        resumePending_ = (cursor > 0);
        ma_sound_stop(&sound_);
    }
    state_ = PlayerState::STOPPED;
}

int Player::pickNextIndex() const {
    const int n = static_cast<int>(songs_.size());
    if (n == 0) {
        return -1;
    }
    switch (mode_) {
        case PlaybackMode::REPEAT_ONE:
            return currentIndex_ < 0 ? 0 : currentIndex_;
        case PlaybackMode::SHUFFLE:
            return randomIndex(n, currentIndex_);
        case PlaybackMode::REPEAT_ALL:
            return currentIndex_ < 0 ? 0 : (currentIndex_ + 1) % n;
        case PlaybackMode::NO_REPEAT:
        default:
            if (currentIndex_ < 0) {
                return 0;
            }
            return (currentIndex_ + 1 < n) ? currentIndex_ + 1 : -1; // -1 => stop
    }
}

int Player::pickPrevIndex() const {
    const int n = static_cast<int>(songs_.size());
    if (n == 0) {
        return -1;
    }
    if (currentIndex_ <= 0) {
        return (mode_ == PlaybackMode::REPEAT_ALL) ? n - 1 : 0;
    }
    return currentIndex_ - 1;
}

void Player::next() {
    if (songs_.empty()) {
        return;
    }
    const int idx = pickNextIndex();
    if (idx < 0) {
        // NO_REPEAT past the last song: stop at the end (no resume).
        stop();
        resumePending_ = false;
        return;
    }
    resumePending_ = false;
    startAt(idx);
}

void Player::previous() {
    if (songs_.empty()) {
        return;
    }
    resumePending_ = false;
    startAt(pickPrevIndex());
}

void Player::tick() {
    if (state_ != PlayerState::PLAYING || !soundLoaded_) {
        return;
    }
    if (ma_sound_at_end(&sound_)) {
        // Song finished: advance based on the active playback mode.
        next();
    }
}

float Player::getCursorSec() const {
    if (!soundLoaded_ || !engineReady_) {
        return 0.0f;
    }
    ma_uint64 frames = 0;
    ma_sound_get_cursor_in_pcm_frames(const_cast<ma_sound*>(&sound_), &frames);
    const ma_uint32 rate = ma_engine_get_sample_rate(const_cast<ma_engine*>(&engine_));
    if (rate == 0) {
        return 0.0f;
    }
    return static_cast<float>(frames) / static_cast<float>(rate);
}

int Player::getDurationSec() const {
    if (soundLoaded_ && engineReady_) {
        ma_uint64 frames = 0;
        ma_sound_get_length_in_pcm_frames(const_cast<ma_sound*>(&sound_), &frames);
        const ma_uint32 rate = ma_engine_get_sample_rate(const_cast<ma_engine*>(&engine_));
        if (rate > 0 && frames > 0) {
            return static_cast<int>(frames / rate);
        }
    }
    return currentSong_ != nullptr ? currentSong_->getDuration() : 0;
}

std::string Player::getTimeString() const {
    const int cursor = static_cast<int>(getCursorSec());
    return utils::formatDuration(cursor) + " / " + utils::formatDuration(getDurationSec());
}

void Player::seekBy(int seconds) {
    if (!soundLoaded_ || !engineReady_) {
        return;
    }
    ma_uint64 cursor = 0;
    ma_uint64 length = 0;
    ma_sound_get_cursor_in_pcm_frames(&sound_, &cursor);
    ma_sound_get_length_in_pcm_frames(&sound_, &length);
    const ma_uint32 rate = ma_engine_get_sample_rate(&engine_);

    ma_int64 newFrame = static_cast<ma_int64>(cursor) +
                        static_cast<ma_int64>(seconds) * static_cast<ma_int64>(rate);
    if (newFrame < 0) {
        newFrame = 0;
    }
    if (static_cast<ma_uint64>(newFrame) >= length) {
        next();
        return;
    }
    ma_sound_seek_to_pcm_frame(&sound_, static_cast<ma_uint64>(newFrame));
}

std::string Player::consumeMessage() {
    std::string msg = message_;
    message_.clear();
    return msg;
}

std::string Player::modeToString(PlaybackMode mode) {
    switch (mode) {
        case PlaybackMode::NO_REPEAT: return "NO_REPEAT";
        case PlaybackMode::REPEAT_ONE: return "REPEAT_ONE";
        case PlaybackMode::REPEAT_ALL: return "REPEAT_ALL";
        case PlaybackMode::SHUFFLE: return "SHUFFLE";
    }
    return "NO_REPEAT";
}

PlaybackMode Player::modeFromString(const std::string& text) {
    const std::string t = utils::trim(text);
    if (utils::iequals(t, "REPEAT_ONE")) return PlaybackMode::REPEAT_ONE;
    if (utils::iequals(t, "REPEAT_ALL")) return PlaybackMode::REPEAT_ALL;
    if (utils::iequals(t, "SHUFFLE")) return PlaybackMode::SHUFFLE;
    return PlaybackMode::NO_REPEAT;
}

std::string Player::stateToString(PlayerState state) {
    switch (state) {
        case PlayerState::PLAYING: return "PLAYING";
        case PlayerState::PAUSED: return "PAUSED";
        case PlayerState::STOPPED: return "STOPPED";
    }
    return "STOPPED";
}
