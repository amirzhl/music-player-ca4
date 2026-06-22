#include "InputHandler.h"

#include <iostream>
#include <string>

#include "Utils.h"

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {
#ifndef _WIN32
// Returns true if stdin has data ready within timeoutMs (POSIX only).
bool readable(int timeoutMs) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
}
#endif

// Shared fallback for piped / redirected input (not an interactive console):
// read one whitespace-delimited token and return its first character so that
// scripted runs still make progress and terminate.
std::string pipedFallback() {
    std::string tok;
    if (!(std::cin >> tok)) {
        std::cin.clear();
        return "q"; // EOF -> behave like "back/quit"
    }
    return tok.substr(0, 1);
}
} // namespace

#ifdef _WIN32
std::string InputHandler::pollKey(int timeoutMs) {
    if (!_isatty(0)) {
        return pipedFallback();
    }
    const int step = 15;
    int elapsed = 0;
    while (true) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 224) { // extended key (arrows, etc.)
                switch (_getch()) {
                    case 72: return "UP";
                    case 80: return "DOWN";
                    case 77: return "RIGHT";
                    case 75: return "LEFT";
                    default: return "";
                }
            }
            if (ch == '\r' || ch == '\n') return "ENTER";
            if (ch == 8) return "BACK";
            if (ch == 27) return "ESC";
            return std::string(1, static_cast<char>(ch));
        }
        if (elapsed >= timeoutMs) {
            return "";
        }
        Sleep(step);
        elapsed += step;
    }
}
#else
std::string InputHandler::pollKey(int timeoutMs) {
    if (!isatty(STDIN_FILENO)) {
        return pipedFallback();
    }
    termios oldt{};
    if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
        return pipedFallback();
    }
    // Raw mode just for this read, restored before returning.
    termios newt = oldt;
    newt.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::string result;
    if (readable(timeoutMs)) {
        unsigned char c = 0;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == '\n' || c == '\r') {
                result = "ENTER";
            } else if (c == 127 || c == 8) {
                result = "BACK";
            } else if (c == 27) {
                // Could be a lone ESC or an arrow-key escape sequence.
                unsigned char a = 0;
                unsigned char b = 0;
                if (readable(5) && read(STDIN_FILENO, &a, 1) == 1 && a == '[' &&
                    readable(5) && read(STDIN_FILENO, &b, 1) == 1) {
                    switch (b) {
                        case 'A': result = "UP"; break;
                        case 'B': result = "DOWN"; break;
                        case 'C': result = "RIGHT"; break;
                        case 'D': result = "LEFT"; break;
                        default: result = "ESC"; break;
                    }
                } else {
                    result = "ESC";
                }
            } else {
                result = std::string(1, static_cast<char>(c));
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return result;
}
#endif

std::string InputHandler::readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cin.clear();
        return "";
    }
    return utils::trim(line);
}
