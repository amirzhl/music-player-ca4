#include "UIRenderer.h"

#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
// Unicode box-drawing pieces (each a fixed 3-byte UTF-8 sequence).
const std::string kH = "\xE2\x95\x90";  // horizontal
const std::string kTL = "\xE2\x95\x94"; // top-left
const std::string kTR = "\xE2\x95\x97"; // top-right
const std::string kBL = "\xE2\x95\x9A"; // bottom-left
const std::string kBR = "\xE2\x95\x9D"; // bottom-right
const std::string kML = "\xE2\x95\xA0"; // mid-left tee
const std::string kMR = "\xE2\x95\xA3"; // mid-right tee
const std::string kV = "\xE2\x95\x91";  // vertical

std::string repeat(const std::string& unit, int times) {
    std::string out;
    out.reserve(unit.size() * static_cast<std::size_t>(times));
    for (int i = 0; i < times; ++i) {
        out += unit;
    }
    return out;
}
} // namespace

void UIRenderer::initConsole() {
#ifdef _WIN32
    // Render Unicode correctly and turn on ANSI escape support.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    ansiEnabled_ = false;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode)) {
        if (SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            ansiEnabled_ = true;
        }
    }
#else
    ansiEnabled_ = true;
#endif
    if (ansiEnabled_) {
        std::cout << "\x1b[2J\x1b[H\x1b[?25l"; // clear, home, hide cursor
        std::cout.flush();
    }
}

void UIRenderer::shutdownConsole() {
    if (ansiEnabled_) {
        std::cout << "\x1b[?25h\n"; // show cursor again
        std::cout.flush();
    }
}

void UIRenderer::showCursor() {
    if (ansiEnabled_) {
        std::cout << "\x1b[?25h";
        std::cout.flush();
    }
}

void UIRenderer::hideCursor() {
    if (ansiEnabled_) {
        std::cout << "\x1b[?25l";
        std::cout.flush();
    }
}

void UIRenderer::fullClear() {
    if (ansiEnabled_) {
        std::cout << "\x1b[2J\x1b[H";
    } else {
#ifdef _WIN32
        std::system("cls");
#else
        std::system("clear");
#endif
    }
    std::cout.flush();
}

void UIRenderer::beginFrame() {
    rows_.clear();
}

void UIRenderer::push(const std::string& line, bool highlighted) {
    rows_.emplace_back(line, highlighted);
}

void UIRenderer::present() {
    if (ansiEnabled_) {
        std::string out = "\x1b[H"; // cursor home -- overwrite, don't clear first
        for (const auto& row : rows_) {
            if (row.second) {
                out += "\x1b[7m" + row.first + "\x1b[0m"; // reverse-video highlight
            } else {
                out += row.first;
            }
            out += "\x1b[K\n"; // clear to end of line, then newline
        }
        out += "\x1b[J"; // clear anything left below (shorter frames)
        std::cout << out;
        std::cout.flush();
    } else {
        // Fallback for terminals without ANSI: a plain clear + redraw.
        fullClear();
        for (const auto& row : rows_) {
            std::cout << row.first << "\n";
        }
        std::cout.flush();
    }
}

std::string UIRenderer::fitLeft(const std::string& text) const {
    std::string content = " " + text;
    if (static_cast<int>(content.size()) > kWidth) {
        content = content.substr(0, static_cast<std::size_t>(kWidth));
    } else {
        content += std::string(static_cast<std::size_t>(kWidth) - content.size(), ' ');
    }
    return content;
}

std::string UIRenderer::fitCentered(const std::string& text) const {
    std::string content = text;
    if (static_cast<int>(content.size()) > kWidth) {
        content = content.substr(0, static_cast<std::size_t>(kWidth));
    }
    const int total = kWidth - static_cast<int>(content.size());
    const int left = total / 2;
    const int right = total - left;
    return std::string(static_cast<std::size_t>(left), ' ') + content +
           std::string(static_cast<std::size_t>(right), ' ');
}

void UIRenderer::boxTop() {
    push(kTL + repeat(kH, kWidth) + kTR);
}

void UIRenderer::boxSeparator() {
    push(kML + repeat(kH, kWidth) + kMR);
}

void UIRenderer::boxBottom() {
    push(kBL + repeat(kH, kWidth) + kBR);
}

void UIRenderer::boxLine(const std::string& text) {
    push(kV + fitLeft(text) + kV);
}

void UIRenderer::boxSelected(const std::string& text) {
    push(kV + fitLeft(text) + kV, true);
}

void UIRenderer::boxCentered(const std::string& text) {
    push(kV + fitCentered(text) + kV);
}

void UIRenderer::header(const std::string& title) {
    boxTop();
    boxCentered(title);
    boxSeparator();
}
