#pragma once
/// \file
/// A <dirent.h> for Windows that carries d_type.
///
/// MinGW has a dirent.h, but its struct dirent has only d_name, and the
/// interpreter tests entry->d_type == DT_DIR when it scans for subroutine
/// files. A member cannot be shimmed onto someone else's struct, so this
/// supplies the whole interface over the Win32 find API instead. On the
/// include path for Windows alone.
// <windows.h> is a heavy neighbour: it defines ERROR and ABSOLUTE as macros,
// and LinuxCNC has enum members by both names (EMC_TASK_EXEC::ERROR and
// DISTANCE_MODE::ABSOLUTE), so including it naively breaks every header that
// follows in the translation unit. NOGDI drops wingdi.h, which is where both
// come from; the rest trims what is pulled in and keeps min/max as functions.
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOGDI
#  define NOGDI
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>

// Belt and braces: NOGDI is honoured by the Windows SDK and by MinGW, but
// these two cost nothing to undo and the failure is a wall of syntax errors in
// unrelated headers.
#undef ERROR
#undef ABSOLUTE

#include <cstdlib>
#include <cstring>

#define DT_UNKNOWN 0
#define DT_DIR     4
#define DT_REG     8

/// one entry of a directory
struct dirent {
    unsigned char d_type;   ///< DT_DIR, DT_REG or DT_UNKNOWN
    char d_name[MAX_PATH];  ///< the file name, without its directory
};

/// an open directory
struct DIR {
    HANDLE handle;          ///< the Win32 find handle
    WIN32_FIND_DATAA data;  ///< what the last find returned
    dirent entry;           ///< what the last readdir() handed back
    bool first;             ///< FindFirstFile already produced an entry
};

/// Open \a path for reading.
/// \param path the directory to read
/// \returns an open directory, or NULL when it cannot be read
inline DIR *opendir(const char *path)
{
    char pattern[MAX_PATH];
    std::snprintf(pattern, sizeof(pattern), "%s\\*", path);

    DIR *dir = (DIR *)std::calloc(1, sizeof(DIR));
    if (!dir)
        return nullptr;

    dir->handle = FindFirstFileA(pattern, &dir->data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        std::free(dir);
        return nullptr;
    }
    dir->first = true;
    return dir;
}

/// Read the next entry of \a dir.
/// \param dir an open directory
/// \returns the entry, or NULL at the end; the entry belongs to \a dir
inline dirent *readdir(DIR *dir)
{
    if (!dir)
        return nullptr;
    if (!dir->first && !FindNextFileA(dir->handle, &dir->data))
        return nullptr;
    dir->first = false;

    std::strncpy(dir->entry.d_name, dir->data.cFileName, MAX_PATH - 1);
    dir->entry.d_name[MAX_PATH - 1] = 0;
    dir->entry.d_type =
        (dir->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
    return &dir->entry;
}

/// Close \a dir.
/// \param dir the directory to close
/// \returns 0
inline int closedir(DIR *dir)
{
    if (dir) {
        FindClose(dir->handle);
        std::free(dir);
    }
    return 0;
}
