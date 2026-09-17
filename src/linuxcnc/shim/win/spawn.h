#pragma once
/// \file
/// A stand-in for POSIX <spawn.h>, which Windows has no equivalent header for.
///
/// rtapi.h includes it; nothing the interpreter does reaches posix_spawn, so
/// this only has to compile. On the include path for Windows alone.
typedef struct { int unused; } posix_spawnattr_t;
typedef struct { int unused; } posix_spawn_file_actions_t;
