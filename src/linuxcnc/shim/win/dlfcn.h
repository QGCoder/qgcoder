#pragma once
/// \file
/// A stand-in for POSIX <dlfcn.h>, which Windows has no equivalent header for.
///
/// LinuxCNC uses it in interp_base.cc for interp_from_shlib(), which loads an
/// alternative interpreter out of a shared library. qgcoder always uses the
/// interpreter compiled into it, so nothing here is ever called - it only has
/// to compile. This header is on the include path for Windows alone.
#define RTLD_GLOBAL 0x100
#define RTLD_LOCAL  0x000
#define RTLD_NOW    0x002
#define RTLD_LAZY   0x001

inline void *dlopen(const char * /*file*/, int /*mode*/) { return nullptr; }
inline void *dlsym(void * /*handle*/, const char * /*name*/) { return nullptr; }
inline int   dlclose(void * /*handle*/) { return 0; }
inline char *dlerror(void) { return const_cast<char *>("no dynamic loading in this build"); }
