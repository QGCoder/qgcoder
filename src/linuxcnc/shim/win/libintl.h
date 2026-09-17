#pragma once
/// \file
/// A stand-in for GNU gettext's <libintl.h>.
///
/// The interpreter wraps its diagnostics in _(), which is gettext(), so that
/// LinuxCNC can translate them. qgcoder ships no message catalogues, and on
/// Windows the real header is worse than useless: it redefines printf and its
/// relatives to libintl_printf and friends, which then want -lintl and a
/// gettext DLL beside the executable. Returning the string unchanged is what
/// gettext does anyway when no catalogue is loaded.
///
/// On the include path for Windows alone; Linux, macOS and WebAssembly use the
/// real header, and there the messages can still be translated.

/// \param msgid the message to translate
/// \returns \a msgid unchanged
inline const char *gettext(const char *msgid) { return msgid; }

/// \param domainname ignored \param msgid the message
/// \returns \a msgid unchanged
inline const char *dgettext(const char *domainname, const char *msgid)
{
    (void)domainname;
    return msgid;
}

/// \param msgid singular form \param msgid_plural plural form \param n the count
/// \returns whichever of the two \a n calls for
inline const char *ngettext(const char *msgid, const char *msgid_plural, unsigned long n)
{
    return (n == 1) ? msgid : msgid_plural;
}

/// \param domainname ignored \returns NULL
inline char *textdomain(const char *domainname) { (void)domainname; return nullptr; }

/// \param domainname ignored \param dirname ignored \returns NULL
inline char *bindtextdomain(const char *domainname, const char *dirname)
{
    (void)domainname;
    (void)dirname;
    return nullptr;
}
