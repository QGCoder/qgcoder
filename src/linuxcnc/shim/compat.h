#pragma once
/// \file
/// Force-included into every vendored LinuxCNC source (see CMakeLists.txt).
///
/// Two things LinuxCNC's own build gives its sources that they do not include
/// for themselves, and which only show up off Linux/glibc.

// Headers the sources use without including: strcasecmp, strncpy, towlower and
// the math constants all appear with no include of their own.
#include <cmath>
#include <cstring>
#include <cwctype>
#if __has_include(<strings.h>)
#  include <strings.h>
#endif

// glibc spells the long-double math constants with an 'l' suffix as an
// extension. MinGW and Emscripten's libc do not have them, and the
// interpreter's arc code uses two. Long double is at least as precise as
// double everywhere qgcoder builds, so falling back loses nothing.
#ifndef M_PIl
#  define M_PIl 3.141592653589793238462643383279502884L
#endif
#ifndef M_PI_2l
#  define M_PI_2l 1.570796326794896619231321691639751442L
#endif

// Two POSIX calls Windows has no equivalent of. The interpreter uses
// realpath() to resolve directories named in a machine .ini - which qgcoder
// never supplies - and link() to keep a backup of the parameter file while it
// is rewritten. Neither is on a path that matters for previewing a tool path.
#ifdef _WIN32
#  include <cstdio>
#  include <cstdlib>

/// Resolve \a path to an absolute one. Windows spells this _fullpath().
/// \param path the path to resolve
/// \param resolved buffer of at least PATH_MAX, or NULL to allocate one
/// \returns the resolved path, or NULL when it does not exist
inline char *realpath(const char *path, char *resolved)
{
    return _fullpath(resolved, path, _MAX_PATH);
}

/// Make \a newpath another name for \a oldpath. Used only to keep a backup of
/// the parameter file, so a copy is as good as a hard link.
/// \param oldpath the existing file
/// \param newpath the name to give it as well
/// \returns 0 on success, -1 on failure
inline int link(const char *oldpath, const char *newpath)
{
    FILE *in = std::fopen(oldpath, "rb");
    if (!in)
        return -1;
    FILE *out = std::fopen(newpath, "wb");
    if (!out) {
        std::fclose(in);
        return -1;
    }
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), in)) > 0) {
        if (std::fwrite(buf, 1, n, out) != n) {
            std::fclose(in);
            std::fclose(out);
            return -1;
        }
    }
    std::fclose(in);
    return std::fclose(out) == 0 ? 0 : -1;
}
#endif // _WIN32
