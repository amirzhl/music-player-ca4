#include <exception>
#include <iostream>

#include "Application.h"

// Program entry point. Paths are fixed and relative to the working directory
// (the project root that contains the Data/ folder), so no CLI arguments are
// used. A top-level try/catch guarantees the program never terminates via an
// uncaught exception. All terminal setup happens inside Application::run.
int main() {
    try {
        Application app;
        app.init();
        app.run();
        app.shutdown();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: unknown exception\n";
        return 1;
    }
    return 0;
}
