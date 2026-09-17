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
#  include <cstring>

/// The file name at the end of \a path, as POSIX basename() gives it. Windows
/// has no <libgen.h>. This is the const form: the interpreter only reads the
/// result, and never expects \a path to be modified.
/// \param path the path to take the last component of
/// \returns a pointer into \a path, after the last separator
inline char *basename(const char *path)
{
    if (!path || !*path)
        return const_cast<char *>(".");
    const char *slash = std::strrchr(path, '/');
    const char *backslash = std::strrchr(path, '\\');
    const char *last = (slash > backslash) ? slash : backslash;
    return const_cast<char *>(last ? last + 1 : path);
}

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

// ---------------------------------------------------------------------------
// std::from_chars for double
//
// interp_read.cc parses every number in a g-code line with std::from_chars.
// libc++ declares the floating-point overload deleted - it has never
// implemented it - and Emscripten is pinned to the version Qt's WebAssembly
// build targets, so it cannot simply be moved forward to one that has it. An
// overload cannot be added beside a deleted declaration either.
//
// So the one call site in interp_read.cc calls this instead. Where the library
// implements from_chars it is used unchanged; where it does not, strtod stands
// in. That is sound for this caller and nowhere else: it has already used
// strspn() to establish that the text is nothing but "+-" followed by digits
// and dots, so none of what separates the two - leading whitespace, hex, inf,
// nan - can appear. strtod follows LC_NUMERIC where from_chars does not, and
// the driver pins that to "C" for the duration of a run.
// ---------------------------------------------------------------------------
#include <charconv>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <system_error>

#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L

/// \param first start of the text
/// \param last  one past its end
/// \param value set to the parsed number on success
/// \returns where parsing stopped, and why if it failed
inline std::from_chars_result qgc_from_chars(const char *first, const char *last,
                                             double &value)
{
    return std::from_chars(first, last, value);
}

#else

/// \copydoc qgc_from_chars
inline std::from_chars_result qgc_from_chars(const char *first, const char *last,
                                             double &value)
{
    const std::string text(first, last);
    char *stopped = nullptr;
    errno = 0;
    const double parsed = std::strtod(text.c_str(), &stopped);

    std::from_chars_result result{};
    if (stopped == text.c_str()) {          // nothing that looked like a number
        result.ptr = first;
        result.ec = std::errc::invalid_argument;
        return result;
    }
    result.ptr = first + (stopped - text.c_str());
    if (errno == ERANGE) {                  // too big or too small for a double
        result.ec = std::errc::result_out_of_range;
        return result;
    }
    value = parsed;
    result.ec = std::errc();
    return result;
}

#endif
