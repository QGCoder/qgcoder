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

// The interpreter calls basename() on a const char*, which is the GNU
// flavour - POSIX's takes a char* and would reject the argument. Only glibc
// declares it, and only out of <string.h>; MinGW, musl (Emscripten) and macOS
// all leave interp_o_word.cc with an undeclared identifier. Passing a
// const char* makes this overload the only viable one even where <libgen.h>
// has declared the POSIX version as well.
#if !defined(__GLIBC__)
#  include <cstring>

/// \param path the path to take the last component of
/// \returns a pointer into \a path, after the last separator
inline char *basename(const char *path)
{
    if (!path || !*path)
        return const_cast<char *>(".");
    const char *slash = std::strrchr(path, '/');
#  ifdef _WIN32
    const char *backslash = std::strrchr(path, '\\');
    if (backslash > slash)
        slash = backslash;
#  endif
    return const_cast<char *>(slash ? slash + 1 : path);
}
#endif // !__GLIBC__

// Two POSIX calls Windows has no equivalent of. The interpreter uses
// realpath() to resolve directories named in a machine .ini - which qgcoder
// never supplies - and link() to keep a backup of the parameter file while it
// is rewritten. Neither is on a path that matters for previewing a tool path.
#ifdef _WIN32
#  include <cstdio>
#  include <cstdlib>
#  include <cstring>
#  include <ctime>
#  include <io.h>
#  include <cstdarg>
#  include <cstdio>

// The POSIX per-thread locale API, which inifile.cc uses to force the C
// locale around strtoll(). Windows has the same idea under different names,
// except for uselocale(): the MSVC runtime installs a locale per thread with
// _configthreadlocale() plus setlocale() rather than by handle. Nothing here
// needs to do that - the driver already pins LC_NUMERIC to "C" for the whole
// run, which is the only reason this code exists - so uselocale() reports
// success and leaves the thread's locale where the driver put it.
#  include <clocale>

typedef _locale_t locale_t;
#  define LC_NUMERIC_MASK LC_NUMERIC
#  define LC_ALL_MASK     LC_ALL

/// \param mask which categories to set, ignored - the whole locale is built
/// \param name the locale to build, e.g. "C"
/// \param base an existing locale to modify, ignored
/// \returns the new locale, or NULL if it could not be built
inline locale_t newlocale(int mask, const char *name, locale_t base)
{
    (void)mask;
    (void)base;
    return _create_locale(LC_ALL, name);
}

/// \param loc the locale to release
inline void freelocale(locale_t loc) { _free_locale(loc); }

/// Report the thread's locale. The driver has already pinned LC_NUMERIC to
/// "C", so there is nothing to install and nothing to restore.
/// \param loc ignored
/// \returns a non-NULL handle, which is all the callers test for
inline locale_t uselocale(locale_t loc)
{
    (void)loc;
    static _locale_t current = _create_locale(LC_ALL, "C");
    return current;
}


// Windows spells the reentrant strtok strtok_s(), with the same signature.
// A macro rather than a function: whether MinGW happens to declare strtok_r
// varies by version, and redeclaring it would then conflict, while redirecting
// the name is right either way.
#ifndef strtok_r
#  define strtok_r strtok_s
#endif

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
/// Resolve \a path to an absolute one. Windows spells this _fullpath().
/// \param path the path to resolve
/// \param resolved a buffer of at least _MAX_PATH, or NULL to allocate one
/// \returns the resolved path, or NULL when it does not exist
inline char *realpath(const char *path, char *resolved)
{
    return _fullpath(resolved, path, _MAX_PATH);
}

/// vasprintf(), a GNU extension that MinGW does not carry. saicanon.cc uses
/// it to format an error of unknown length.
///
/// This was left out of an earlier round on the assumption mingw-w64 provided
/// it; the build says otherwise. It had been hidden until now because
/// libintl.h was rewriting the name to libintl_vasprintf, which failed at link
/// instead of at compile.
/// \param strp set to the allocated string on success; the caller frees it
/// \param fmt printf format
/// \param ap the arguments
/// \returns the number of characters written, or -1 on failure
inline int vasprintf(char **strp, const char *fmt, va_list ap)
{
    va_list measure;
    va_copy(measure, ap);
    const int len = std::vsnprintf(nullptr, 0, fmt, measure);
    va_end(measure);
    if (len < 0)
        return -1;

    *strp = (char *)std::malloc((size_t)len + 1);
    if (!*strp)
        return -1;
    return std::vsnprintf(*strp, (size_t)len + 1, fmt, ap);
}

/// localtime() for a plain long.
///
/// MinGW's struct timeval carries a 32-bit long tv_sec while time_t is 64-bit,
/// so the interpreter's logging code passes a long* where localtime() wants a
/// const time_t*. Overloading on the narrower type converts and forwards,
/// which is what the caller means; the call inside picks the real localtime()
/// because tt is a time_t.
/// \param t seconds since the epoch
/// \returns the broken-down local time
inline struct tm *localtime(const long *t)
{
    const time_t tt = *t;
    return localtime(&tt);
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

// Apple is excluded regardless of the feature test. libc++ ships the
// floating-point from_chars but annotates it as introduced in macOS 13.4, and
// qgcoder targets macOS 11 (Big Sur), so using it is a hard error - "is
// unavailable" - not a silent fallback. The feature-test macro says nothing
// about the deployment target, so it cannot answer this on its own.
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L && !defined(__APPLE__)

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

// fdatasync(), which the interpreter calls after rewriting the parameter file,
// is a Linux interface. Windows spells it _commit(); macOS has only fsync(),
// which also flushes the metadata and so does more than asked, not less.
#if defined(_WIN32)
#  include <io.h>
/// \param fd the file descriptor to flush
/// \returns 0 on success, -1 on failure
inline int fdatasync(int fd) { return _commit(fd); }
#elif defined(__APPLE__)
#  include <unistd.h>
/// \copydoc fdatasync
inline int fdatasync(int fd) { return fsync(fd); }
#endif
