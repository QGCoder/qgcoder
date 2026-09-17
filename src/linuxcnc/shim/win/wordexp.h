#pragma once
/// \file
/// A stand-in for POSIX <wordexp.h>, which Windows has no equivalent of.
///
/// rs274ngc_pre.cc uses wordexp() to expand shell syntax - ~, $VAR, globs - in
/// directory names read from a machine .ini. qgcoder never supplies one, so
/// these paths are not reached; this hands the word back unchanged so that the
/// caller sees a valid, one-word result either way. Windows only.
#include <cstdlib>
#include <cstring>

/// the result of a wordexp() call
typedef struct {
    size_t we_wordc;   ///< how many words came back
    char **we_wordv;   ///< the words themselves, NULL terminated
    size_t we_offs;    ///< unused
} wordexp_t;

#define WRDE_NOCMD  (1 << 2)
#define WRDE_APPEND (1 << 0)
#define WRDE_UNDEF  (1 << 3)

/// Hand \a words back as a single word, unexpanded.
/// \param words the text to expand
/// \param result filled in with the one word
/// \param flags ignored
/// \returns 0, or -1 if it could not allocate
inline int wordexp(const char *words, wordexp_t *result, int flags)
{
    (void)flags;
    if (!result)
        return -1;
    result->we_wordv = (char **)std::calloc(2, sizeof(char *));
    if (!result->we_wordv)
        return -1;
    result->we_wordv[0] = _strdup(words ? words : "");
    result->we_wordv[1] = nullptr;
    result->we_wordc = 1;
    result->we_offs = 0;
    return 0;
}

/// Release what wordexp() allocated.
/// \param result the result to free
inline void wordfree(wordexp_t *result)
{
    if (!result || !result->we_wordv)
        return;
    for (size_t i = 0; i < result->we_wordc; ++i)
        std::free(result->we_wordv[i]);
    std::free(result->we_wordv);
    result->we_wordv = nullptr;
    result->we_wordc = 0;
}
