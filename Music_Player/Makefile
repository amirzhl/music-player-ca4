# Music Player - build file
# Build:   make         (Linux / macOS)   |  mingw32-make         (Windows + MinGW)
# Run:     make run     (Linux / macOS)   |  mingw32-make run     (Windows)
# Clean:   make clean   (Linux / macOS)   |  mingw32-make clean   (Windows)

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRCDIR   := src
SRC      := $(wildcard $(SRCDIR)/*.cpp)
OBJ      := $(SRC:.cpp=.o)

ifeq ($(OS),Windows_NT)
    # ---- Windows (MinGW) ----
    # Detected through the built-in OS variable, so we never call `uname`
    # (which does not exist on a plain Windows shell).
    TARGET  := music_player.exe
    # miniaudio uses the WASAPI backend on Windows; it needs the COM and
    # multimedia libraries.
    LDFLAGS := -lole32 -lwinmm
else
    # ---- Linux / macOS ----
    TARGET  := music_player
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        # On Linux miniaudio dlopen()s the ALSA/PulseAudio backends at runtime.
        LDFLAGS := -lpthread -lm -ldl
    else
        LDFLAGS :=
    endif
endif

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

ifeq ($(OS),Windows_NT)
run: $(TARGET)
	$(TARGET)

clean:
	-del /Q /F $(SRCDIR)\*.o $(TARGET) 2>NUL
else
run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
endif
