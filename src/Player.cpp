// The MINIAUDIO_IMPLEMENTATION macro must be defined in EXACTLY ONE .cpp file.
// Defining it here (and nowhere else) avoids "multiple definition" linker errors.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "Player.h"

#include <cstring>
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
    // Create the playback device ONCE. It stays alive (and running) for the
    // whole lifetime of the player; track changes never touch the device, only
    // the decoder it reads from. Forcing a fixed output format means every
    // decoder is configured to match it, so no per-track device juggling is
    // needed.
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = channels_;
    config.sampleRate = sampleRate_;
    config.dataCallback = &Player::dataCallback;
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, &device_) == MA_SUCCESS) {
        // Keep the device running for the whole session. The callback simply
        // outputs silence whenever nothing is playing.
        deviceReady_ = (ma_device_start(&device_) == MA_SUCCESS);
        if (!deviceReady_) {
            ma_device_uninit(&device_);
        }
    }

    if (!deviceReady_) {
        // No usable audio backend/device was found (typical on WSL, plain SSH
        // sessions, containers, or VMs without a sound device). The player
        // still runs as a complete state machine -- it just makes no sound.
        std::cerr << "[audio] could not initialise an audio device -- "
                     "running in silent mode (no audio device detected).\n";
    }
}

Player::~Player() {
    // Stop the device FIRST. ma_device_uninit() stops and joins the audio
    // thread, so once it returns the callback can no longer run -- only then is
    // it safe to free the decoder it was reading from. This ordering is what
    // guarantees a clean shutdown with no lingering process.
    if (deviceReady_) {
        ma_device_uninit(&device_);
        deviceReady_ = false;
    }
    if (decoder_ != nullptr) {
        ma_decoder_uninit(decoder_);
        delete decoder_;
        decoder_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Audio thread
// ---------------------------------------------------------------------------
void Player::dataCallback(ma_device* device, void* output, const void* input,
                          ma_uint32 frameCount) {
    (void)input;
    auto* self = static_cast<Player*>(device->pUserData);
    if (self != nullptr) {
        self->mixAudio(output, frameCount);
    }
}

void Player::mixAudio(void* output, ma_uint32 frameCount) {
    const std::size_t totalSamples =
        static_cast<std::size_t>(frameCount) * channels_;
    auto* out = static_cast<float*>(output);

    // Short, bounded critical section: read frames from the current decoder.
    // The main thread only ever swaps decoder_ (and frees the old one) while
    // holding this same lock, so the pointer we read here is always valid for
    // the duration of the call.
    std::lock_guard<std::mutex> lock(audioMutex_);

    if (!playing_.load() || decoder_ == nullptr) {
        std::memset(out, 0, totalSamples * sizeof(float));
        return;
    }

    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(decoder_, out, frameCount, &framesRead);

    if (framesRead < frameCount) {
        // Reached the end of the song: pad the rest with silence and signal the
        // main loop (via tick()) to advance. We do NOT touch playlist state on
        // the audio thread -- that stays entirely on the main thread.
        std::memset(out + framesRead * channels_, 0,
                    static_cast<std::size_t>(frameCount - framesRead) *
                        channels_ * sizeof(float));
        playing_.store(false);
        atEnd_.store(true);
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
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
}

void Player::selectByPath(const std::string& filePath) {
    for (std::size_t i = 0; i < songs_.size(); ++i) {
        if (songs_[i] != nullptr && songs_[i]->getFilePath() == filePath) {
            currentIndex_ = static_cast<int>(i);
            currentSong_ = songs_[i];
            state_ = PlayerState::STOPPED;
            return;
        }
    }
}

void Player::unloadDecoder() {
    // Detach the current decoder under the lock so the audio thread can never
    // be mid-read on it, then free it OUTSIDE the lock. Because the device is
    // never destroyed here, there is no teardown handshake with the backend to
    // deadlock on -- swapping a passive data source is always safe and fast.
    ma_decoder* old = nullptr;
    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        playing_.store(false);
        atEnd_.store(false);
        old = decoder_;
        decoder_ = nullptr;
    }
    if (old != nullptr) {
        ma_decoder_uninit(old);
        delete old;
    }
}

void Player::loadAndStart(Song* song) {
    if (song == nullptr) {
        unloadDecoder();
        return;
    }
    if (!deviceReady_) {
        message_ = "Audio device unavailable -- playing in silent mode.";
        return;
    }

    // Open the new decoder BEFORE touching the live one, configured to match
    // the device's output format so no per-track conversion juggling is needed.
    ma_decoder_config config =
        ma_decoder_config_init(ma_format_f32, channels_, sampleRate_);
    auto* fresh = new ma_decoder();
    const ma_result result =
        ma_decoder_init_file(song->getFilePath().c_str(), &config, fresh);
    if (result != MA_SUCCESS) {
        delete fresh;
        message_ = "Could not decode audio file: " + song->getFilePath();
        return; // leaves the previous decoder untouched
    }

    // Swap in the new decoder under the lock (pointer swap only -> the audio
    // thread never observes a partially built decoder), then free the old one
    // outside the lock. Only one decoder is ever alive at a time, so memory
    // stays bounded to a single track and there is no leak.
    ma_decoder* old = nullptr;
    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        old = decoder_;
        decoder_ = fresh;
        atEnd_.store(false);
        playing_.store(true);
    }
    if (old != nullptr) {
        ma_decoder_uninit(old);
        delete old;
    }
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

    // Normalise and try up to n songs, skipping those that cannot be played.
    int idx = ((index % n) + n) % n;
    for (int attempt = 0; attempt < n; ++attempt) {
        Song* song = songs_[idx];
        if (song != nullptr && fileExists(song->getFilePath())) {
            const bool wasLoaded = (decoder_ != nullptr);
            loadAndStart(song);
            const bool nowLoaded = (decoder_ != nullptr);
            // Treat the song as "playing" if a decoder actually loaded -- or if
            // we are in silent mode (no device), where the state machine still
            // runs without sound. A file that exists but fails to decode is
            // skipped just like a missing one, so one bad track can never
            // freeze playback on a "PLAYING" screen that never advances.
            const bool decoded = nowLoaded && (nowLoaded != wasLoaded || playing_.load());
            if (decoded || !deviceReady_) {
                currentIndex_ = idx;
                currentSong_ = song;
                state_ = PlayerState::PLAYING;
                return true;
            }
            message_ = "Could not decode audio file, skipping: " + song->getFilePath();
        } else if (song != nullptr) {
            message_ = "Audio file not found, skipping: " + song->getFilePath();
        }
        idx = (idx + 1) % n;
    }

    message_ = "No playable audio files in this playlist.";
    state_ = PlayerState::STOPPED;
    return false;
}

bool Player::playIndex(int index) {
    return startAt(index);
}

void Player::play() {
    if (songs_.empty()) {
        message_ = "Playlist is empty.";
        return;
    }
    // Per the project spec, "play" starts the current song from the beginning.
    // Continuing from a paused position is handled by resume(), not play().
    const int idx = currentIndex_ < 0 ? 0 : currentIndex_;
    startAt(idx);
}

void Player::pause() {
    if (state_ != PlayerState::PLAYING) {
        return; // silently ignored when nothing is playing
    }
    // Just tell the callback to stop pulling frames; the decoder cursor stays
    // exactly where it is, so resume() continues from the same spot.
    playing_.store(false);
    state_ = PlayerState::PAUSED;
}

void Player::resume() {
    if (state_ != PlayerState::PAUSED) {
        return; // silently ignored
    }
    if (decoder_ != nullptr) {
        playing_.store(true);
    }
    state_ = PlayerState::PLAYING;
}

void Player::stop() {
    // Per the project spec: "stop" halts playback AND resets the position to
    // zero, so the next play() begins the song from the start.
    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        playing_.store(false);
        atEnd_.store(false);
        if (decoder_ != nullptr) {
            ma_decoder_seek_to_pcm_frame(decoder_, 0);
        }
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
        // NO_REPEAT past the last song: stop at the end.
        stop();
        return;
    }
    startAt(idx);
}

void Player::previous() {
    if (songs_.empty()) {
        return;
    }
    startAt(pickPrevIndex());
}

void Player::tick() {
    if (state_ != PlayerState::PLAYING) {
        return;
    }
    // The audio thread sets atEnd_ when the current song runs out. We react on
    // the main thread so all playlist/state changes stay single-threaded.
    if (atEnd_.exchange(false)) {
        next();
    }
}

float Player::getCursorSec() const {
    std::lock_guard<std::mutex> lock(audioMutex_);
    if (decoder_ == nullptr || sampleRate_ == 0) {
        return 0.0f;
    }
    ma_uint64 frames = 0;
    ma_decoder_get_cursor_in_pcm_frames(decoder_, &frames);
    return static_cast<float>(frames) / static_cast<float>(sampleRate_);
}

int Player::getDurationSec() const {
    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        if (decoder_ != nullptr && sampleRate_ > 0) {
            ma_uint64 frames = 0;
            if (ma_decoder_get_length_in_pcm_frames(decoder_, &frames) == MA_SUCCESS &&
                frames > 0) {
                return static_cast<int>(frames / sampleRate_);
            }
        }
    }
    return currentSong_ != nullptr ? currentSong_->getDuration() : 0;
}

std::string Player::getTimeString() const {
    const int cursor = static_cast<int>(getCursorSec());
    return utils::formatDuration(cursor) + " / " + utils::formatDuration(getDurationSec());
}

void Player::seekBy(int seconds) {
    bool reachedEnd = false;
    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        if (decoder_ == nullptr || !deviceReady_) {
            return;
        }
        ma_uint64 cursor = 0;
        ma_uint64 length = 0;
        ma_decoder_get_cursor_in_pcm_frames(decoder_, &cursor);
        ma_decoder_get_length_in_pcm_frames(decoder_, &length);

        // Fall back to the library duration if the decoder cannot report a
        // length, so seeking still has a sane upper bound.
        if (length == 0) {
            const int durSec = currentSong_ != nullptr ? currentSong_->getDuration() : 0;
            length = static_cast<ma_uint64>(durSec) * sampleRate_;
        }

        ma_int64 newFrame = static_cast<ma_int64>(cursor) +
                            static_cast<ma_int64>(seconds) *
                                static_cast<ma_int64>(sampleRate_);
        if (newFrame < 0) {
            newFrame = 0;
        }
        if (length > 0 && static_cast<ma_uint64>(newFrame) >= length) {
            reachedEnd = true; // handle outside the lock (next() re-locks)
        } else {
            ma_decoder_seek_to_pcm_frame(decoder_, static_cast<ma_uint64>(newFrame));
        }
    }
    // next() acquires audioMutex_ again, so it MUST be called after releasing
    // the lock above -- std::mutex is not recursive.
    if (reachedEnd) {
        next();
    }
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
