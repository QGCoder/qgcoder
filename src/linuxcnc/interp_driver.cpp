/***************************************************************************
 *   In-process driver for the embedded LinuxCNC RS274NGC interpreter.     *
 *   Copyright (C) 2026 by Jakob Flierl                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This file does what emc/sai/driver.cc does in LinuxCNC: it drives the *
 *   interpreter over a file. It returns errors instead of writing to      *
 *   stderr and calling exit(), and hands each canonical command to a      *
 *   callback instead of printing it.                                      *
 ***************************************************************************/

/// \file
/// \see rs274ngc::Interpreter

#include "interp_driver.hpp"

#include "interp_base.hh"
#include "rs274ngc.hh"
#include "rs274ngc_interp.hh"
#include "rs274ngc_return.hh"
#include "interp_return.hh"
#include "saicanon.hh"
#include "tooldata.hh"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#include <locale.h>

/// where the canonical commands are printed; saicanon.cc writes through this
extern FILE *_outfile;
/// the interpreter saicanon.cc reads the current g-code line from
extern InterpBase *pinterp;
/// where the interpreter reads and rewrites its parameters; the canon layer
/// hands this back through GET_EXTERNAL_PARAMETER_FILE_NAME()
extern char _parameter_file_name[PARAMETER_FILE_NAME_LENGTH];

namespace rs274ngc {

namespace {

/// Only one interpretation at a time: the interpreter and the canon layer both
/// keep their whole state in file-scope globals.
std::mutex g_interpMutex;

/// guards \ref g_parameterDir
std::mutex g_parameterDirMutex;
/// \see setParameterFileDirectory()
std::string g_parameterDir;

/// Clears _outfile on the way out, so the canon layer is never left pointing at
/// a stream that has been closed - every early return below relies on it.
struct OutfileReset {
    ~OutfileReset() { _outfile = NULL; }
};


/// \brief Forces LC_NUMERIC to "C" for as long as it is alive.
///
/// The interpreter reads and prints every coordinate with strtod()/fprintf(),
/// which follow the locale. As a separate process it always ran in the "C"
/// locale; linked into a Qt application it would instead inherit whatever Qt
/// set, and in a locale that writes 1,5 for one-and-a-half every fractional
/// coordinate in the g-code would be silently truncated. uselocale() changes
/// only this thread, so the interpreter can run on a worker without disturbing
/// the number formatting the GUI thread is using.
#ifdef _WIN32
class CNumericLocale {
public:
    CNumericLocale() : threadMode(-1)
    {
        // Windows has no uselocale(); _configthreadlocale() is what makes
        // setlocale() apply to the calling thread alone.
        threadMode = _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
        const char *current = setlocale(LC_NUMERIC, NULL);
        if (current != NULL)
            previous = current;
        setlocale(LC_NUMERIC, "C");
    }
    ~CNumericLocale()
    {
        if (!previous.empty())
            setlocale(LC_NUMERIC, previous.c_str());
        if (threadMode != -1)
            _configthreadlocale(threadMode);
    }
private:
    CNumericLocale(const CNumericLocale &);
    CNumericLocale &operator=(const CNumericLocale &);
    std::string previous;
    int threadMode;
};
#else
class CNumericLocale {
public:
    CNumericLocale() : applied((locale_t)0), previous((locale_t)0)
    {
        applied = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
        if (applied != (locale_t)0)
            previous = uselocale(applied);
    }
    ~CNumericLocale()
    {
        if (previous != (locale_t)0)
            uselocale(previous);
        if (applied != (locale_t)0)
            freelocale(applied);
    }
private:
    CNumericLocale(const CNumericLocale &);
    CNumericLocale &operator=(const CNumericLocale &);
    locale_t applied;
    locale_t previous;
};
#endif


/// \brief The stream the canon layer prints through, readable as it grows.
///
/// open_memstream() is exactly this - a FILE* whose backing buffer always
/// holds everything written so far - but it is POSIX and Windows has no
/// equivalent. There the same shape comes from a temporary file read back as
/// it grows, which costs nothing that matters: even a large program's
/// canonical output is small, and it never outlives the run. tmpfile() would
/// have been shorter, but on Windows it wants to create its file in the root
/// of the current drive, which an unprivileged process may not do.
class CanonOutput {
public:
    CanonOutput() : m_file(NULL)
#ifdef _WIN32
        , m_read(0)
#else
        , m_buffer(NULL), m_length(0)
#endif
    {}
    ~CanonOutput() { close(); }

    bool open()
    {
#ifdef _WIN32
        char *name = _tempnam(NULL, "qgcoder-");
        if (name == NULL)
            return false;
        m_path = name;
        free(name);
        m_file = fopen(m_path.c_str(), "wb+");
#else
        m_file = open_memstream(&m_buffer, &m_length);
#endif
        return m_file != NULL;
    }

    FILE *file() const { return m_file; }

    /// push out what the interpreter has written, so data() and size() cover it
    void sync()
    {
        if (m_file == NULL)
            return;
        fflush(m_file);
#ifdef _WIN32
        // fflush() leaves the position at the end of what has been written,
        // which is also where the bytes we have not read yet stop.
        const long end = ftell(m_file);
        if (end < 0 || (size_t)end <= m_read)
            return;
        m_text.resize((size_t)end);
        // C requires a seek between reading and writing the same stream, which
        // the two fseek() calls around the read take care of.
        fseek(m_file, (long)m_read, SEEK_SET);
        m_read += fread(&m_text[m_read], 1, (size_t)end - m_read, m_file);
        m_text.resize(m_read);
        fseek(m_file, 0, SEEK_END);
#endif
    }

    const char *data() const
    {
#ifdef _WIN32
        return m_text.c_str();
#else
        return m_buffer;
#endif
    }

    size_t size() const
    {
#ifdef _WIN32
        return m_text.size();
#else
        return m_length;
#endif
    }

    void close()
    {
        if (m_file != NULL) {
            fclose(m_file);
            m_file = NULL;
        }
#ifdef _WIN32
        if (!m_path.empty()) {
            remove(m_path.c_str());
            m_path.clear();
        }
#else
        free(m_buffer);
        m_buffer = NULL;
        m_length = 0;
#endif
    }

private:
    CanonOutput(const CanonOutput &);
    CanonOutput &operator=(const CanonOutput &);

    FILE *m_file;
#ifdef _WIN32
    std::string m_path;
    std::string m_text;
    size_t m_read;
#else
    char *m_buffer;
    size_t m_length;
#endif
};


/// Hand every complete line that the canon layer has written so far to
/// \a onLine, and remember how much of \a buffer has been consumed.
int drain(const char *buffer, size_t length, size_t *consumed,
          const LineHandler &onLine)
{
    int emitted = 0;
    while (*consumed < length) {
        const char *start = buffer + *consumed;
        const char *nl = (const char *)memchr(start, '\n', length - *consumed);
        if (nl == NULL)
            break;
        onLine(std::string(start, nl - start));
        *consumed = (nl - buffer) + 1;
        emitted++;
    }
    return emitted;
}


/// Builds the message for an interpreter status code, with the offending line
/// and the call stack the interpreter kept, when it has them.
/// \param interp the interpreter that reported the status
/// \param errorCode a status other than INTERP_OK
/// \returns the message to show the user
std::string errorTextFor(Interp &interp, int errorCode)
{
    char buffer[LINELEN];
    std::string text;

    interp.error_text(errorCode, buffer, sizeof(buffer));
    text = (buffer[0] == 0) ? "Unknown error, bad error code" : buffer;

    interp.line_text(buffer, sizeof(buffer));
    if (buffer[0] != 0) {
        text += "\n";
        text += buffer;
    }

    for (int k = 0; k < 10; ++k) {
        interp.stack_name(k, buffer, sizeof(buffer));
        if (buffer[0] == 0)
            break;
        text += "\n  in ";
        text += buffer;
    }
    return text;
}

} // namespace

void setParameterFileDirectory(const std::string &dir)
{
    std::lock_guard<std::mutex> lock(g_parameterDirMutex);
    g_parameterDir = dir;
}

std::string defaultParameterFile()
{
    std::lock_guard<std::mutex> lock(g_parameterDirMutex);
    std::string dir = g_parameterDir;
    if (dir.empty()) {
        const char *tmp = getenv("TMPDIR");
        dir = (tmp != NULL && tmp[0] != 0) ? tmp : "/tmp";
    }
    if (!dir.empty() && dir[dir.size() - 1] != '/')
        dir += '/';
    return dir + "rs274ngc.var";
}

Result Interpreter::interpretFile(const std::string &ngcFile,
                                  const LineHandler &onLine,
                                  const AbortHandler &shouldAbort)
{
    Result result;

    if (!onLine) {
        result.error = "No output handler given to the interpreter";
        return result;
    }

    std::lock_guard<std::mutex> lock(g_interpMutex);
    CNumericLocale cLocale;

    // The canon layer prints through _outfile. A stream we can read back as it
    // fills lets us pick complete lines out of it as they appear.
    CanonOutput out;
    if (!out.open()) {
        result.error = "Cannot open the interpreter output stream";
        return result;
    }
    _outfile = out.file();
    OutfileReset outfileReset;

    // The tool table lives in an array the canon layer owns; the interpreter
    // reaches it through tooldata. Without this, every tool lookup fails.
    tooldata_init(false);
    tool_nml_register((CANON_TOOL_TABLE *)&_sai._tools);
    if (!toolTable.empty())
        tooldata_load(toolTable.c_str());

    // Interp's constructor has to run inside the lock: it touches the same
    // globals, and saicanon dereferences pinterp without checking it.
    Interp interp;
    pinterp = &interp;

    // The interpreter asks the canon layer where its parameter file lives, at
    // init and again at exit. Left empty it writes rs274ngc.var into whatever
    // the working directory happens to be, and the rename at exit fails.
    const std::string varFile =
        parameterFile.empty() ? defaultParameterFile() : parameterFile;
    if (varFile.size() < sizeof(_parameter_file_name)) {
        std::strncpy(_parameter_file_name, varFile.c_str(),
                     sizeof(_parameter_file_name) - 1);
        _parameter_file_name[sizeof(_parameter_file_name) - 1] = 0;
    }
    interp.set_loglevel(0);

    size_t consumed = 0;
    int status = interp.init();
    if (status != INTERP_OK) {
        result.error = errorTextFor(interp, status);
        pinterp = NULL;
        return result;
    }

    status = interp.open(ngcFile.c_str());
    if (status != INTERP_OK) {
        result.error = errorTextFor(interp, status);
        interp.exit();
        pinterp = NULL;
        return result;
    }

    // This is interpret_from_file() from driver.cc with do_next == 2 ("stop on
    // error"), minus the MDI prompts that needed a terminal.
    bool failed = false;
    for (;;) {
        if (shouldAbort && shouldAbort()) {
            result.aborted = true;
            break;
        }

        status = interp.read();
        if (status == INTERP_ENDFILE)
            break;
        if (status != INTERP_OK && status != INTERP_EXECUTE_FINISH) {
            result.error = errorTextFor(interp, status);
            failed = true;
            break;
        }

        status = interp.execute();
        out.sync();
        result.canonLines += drain(out.data(), out.size(), &consumed, onLine);

        if (status == INTERP_EXIT) {
            result.reachedEnd = true;
            break;
        }
        if (status != INTERP_OK && status != INTERP_EXECUTE_FINISH) {
            result.error = errorTextFor(interp, status);
            failed = true;
            break;
        }
    }

    interp.close();
    interp.exit();
    pinterp = NULL;

    out.sync();
    result.canonLines += drain(out.data(), out.size(), &consumed, onLine);
    if (consumed < out.size())                    // a final line with no newline
        onLine(std::string(out.data() + consumed, out.size() - consumed));

    result.ok = !failed;
    return result;
}

} // namespace rs274ngc
