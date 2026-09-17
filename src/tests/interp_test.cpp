/// \file
/// Interprets a g-code file and prints every canonical command, so the result
/// can be compared against a recorded one.
///
/// This is what LinuxCNC's own `rs274` does, cut down to what the tests need:
/// no tool table, no MDI, no options. The output is the interpreter's own
/// canonical form, which is also what g2m parses to build the tool path, so a
/// difference here is a difference in what qgcoder would draw.

#include <cstdio>
#include <string>

#include "interp_driver.hpp"

/// \param argc argument count
/// \param argv the g-code file to interpret, then where to keep its parameter file
/// \returns 0 when the file interpreted without error, 1 otherwise
int main(int argc, char **argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: interp_test FILE.ngc [PARAMETER_FILE]\n");
        return 2;
    }

    rs274ngc::Interpreter interp;
    // Somewhere writable and per-run: the interpreter rewrites this at exit,
    // and two tests running at once must not share it.
    interp.setParameterFile(argc > 2 ? argv[2] : "rs274ngc.var");

    const rs274ngc::Result result = interp.interpretFile(
        argv[1],
        [](const std::string &line) { std::printf("%s\n", line.c_str()); });

    if (!result.ok) {
        std::fprintf(stderr, "interpreter error: %s\n", result.error.c_str());
        return 1;
    }
    if (!result.reachedEnd) {
        std::fprintf(stderr, "interpreter did not reach the end of the program\n");
        return 1;
    }
    return 0;
}
