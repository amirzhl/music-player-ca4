#pragma once

#include <string>

// Centralised console input.
//
// The whole UI is driven by single key presses (no Enter needed). pollKey waits
// up to a timeout for one key and returns it as a small token; readLine is used
// only for the rare case of typing free text (the search box).
class InputHandler {
public:
    // Waits up to timeoutMs for a single key press and returns it as a token:
    //   - printable keys: the character itself, e.g. "a", "1", "/"
    //   - "ENTER", "BACK" (backspace), "ESC"
    //   - "UP", "DOWN", "LEFT", "RIGHT" (arrow keys)
    //   - "" if no key was pressed within the timeout.
    // Redirected/piped input falls back to a blocking read so automated runs
    // still terminate instead of looping forever.
    std::string pollKey(int timeoutMs);

    // Reads a whole line of free text (used for the search box). Blocking.
    std::string readLine(const std::string& prompt);
};
