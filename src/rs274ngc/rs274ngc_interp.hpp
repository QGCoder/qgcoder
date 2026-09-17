/***************************************************************************
 *   In-process driver for the embedded NIST RS274NGC interpreter.         *
 *   Copyright (C) 2026 by Jakob Flierl                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 ***************************************************************************/

/// \file
/// In-process driver for the embedded NIST RS274NGC interpreter.

#ifndef RS274NGC_INTERP_HPP
#define RS274NGC_INTERP_HPP

#include <functional>
#include <string>

namespace rs274ngc {

/// \brief What one interpreter run produced.
struct Result {
    /// true when the whole file was interpreted without an interpreter error
    bool ok = false;
    /// true when the interpreter reached M2/M30 (PROGRAM_END)
    bool reachedEnd = false;
    /// true when the abort callback stopped the run early
    bool aborted = false;
    /// interpreter error text (message, offending line, and call stack), empty when ok
    std::string error;
    /// number of canonical command lines handed to the line callback
    int canonLines = 0;
};

/// called once per canonical output line, without the trailing newline
typedef std::function<void(const std::string &line)> LineHandler;
/// called between g-code lines; return true to stop interpreting
typedef std::function<bool()> AbortHandler;

/// \brief Runs the RS274NGC interpreter inside this process.
///
/// This replaces the stand-alone "rs274" executable that qgcoder used to drive
/// over a pipe. The NIST interpreter keeps all of its state in file-scope
/// globals, so only one run can be in flight at a time; interpretFile()
/// serialises callers on an internal mutex and is safe to call from a worker
/// thread.
class Interpreter {
public:
    /// Path of the parameter file (rs274ngc.var) to read at init and rewrite at
    /// exit. If the file does not exist it is created from the built-in
    /// default. When left empty, defaultParameterFile() is used.
    /// \param path the parameter file to use
    void setParameterFile(const std::string &path) { parameterFile = path; }

    /// Path of the tool table to load. An empty path, a missing file, or a file
    /// that cannot be parsed falls back to the built-in default tool table.
    /// \param path the tool table to load
    void setToolTable(const std::string &path) { toolTable = path; }

    /// Interpret \a ngcFile, reporting every canonical command to \a onLine.
    /// \a shouldAbort is polled once per g-code line and may be empty.
    Result interpretFile(const std::string &ngcFile,
                         const LineHandler &onLine,
                         const AbortHandler &shouldAbort = AbortHandler());

private:
    std::string parameterFile;  ///< \see setParameterFile()
    std::string toolTable;      ///< \see setToolTable()
};

/// Default parameter file location: rs274ngc.var next to the other application
/// data, created from the built-in default on first use.
std::string defaultParameterFile();

/// Override the directory the default parameter file lives in. qgcoder points
/// this at QStandardPaths::AppDataLocation; without it the system temp
/// directory is used.
void setParameterFileDirectory(const std::string &dir);

} // namespace rs274ngc

#endif // RS274NGC_INTERP_HPP
