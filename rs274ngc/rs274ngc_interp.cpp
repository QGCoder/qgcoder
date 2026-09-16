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
 *                                                                         *
 *   This file takes the place of the interpreter's own driver.cc: it does
 *   what main() there did, but as a callable function that returns errors
 *   instead of writing to stderr and calling exit(), and that hands each
 *   canonical command to a callback instead of printing it to stdout.
 ***************************************************************************/

#include "rs274ngc_interp.hpp"

#include "canon.hh"
#include "rs274ngc.hh"
#include "rs274ngc_return.hh"

#include <cctype>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#include <locale.h>

extern CANON_TOOL_TABLE _tools[];                 /* in canon_pre.cc */
extern int _tool_max;                             /* in canon_pre.cc */
extern char _parameter_file_name[];               /* in canon_pre.cc */

/// where the canonical commands are printed; canon_pre.cc writes through this
FILE *_outfile = NULL;

namespace rs274ngc {

namespace {

/// Only one interpretation at a time: the interpreter kernel and the canon
/// layer both keep their whole state in file-scope globals.
std::mutex g_interpMutex;

/// Directory for the parameter file, see setParameterFileDirectory().
std::string g_parameterDir;
std::mutex g_parameterDirMutex;

/// The data lines of the stock rs274ngc.var. Every one of these parameters is
/// required by rs274ngc_restore_parameters(); 5220 (the work coordinate system
/// index) must be between 1 and 9.
const char *const kDefaultParameters =
    "5161\t0.000000\n5162\t0.000000\n5163\t0.000000\n5164\t0.000000\n"
    "5165\t0.000000\n5166\t0.000000\n5181\t0.000000\n5182\t0.000000\n"
    "5183\t0.000000\n5184\t0.000000\n5185\t0.000000\n5186\t0.000000\n"
    "5211\t0.000000\n5212\t0.000000\n5213\t0.000000\n5214\t0.000000\n"
    "5215\t0.000000\n5216\t0.000000\n5220\t1.000000\n5221\t0.000000\n"
    "5222\t0.000000\n5223\t0.000000\n5224\t0.000000\n5225\t0.000000\n"
    "5226\t0.000000\n5241\t0.000000\n5242\t0.000000\n5243\t0.000000\n"
    "5244\t0.000000\n5245\t0.000000\n5246\t0.000000\n5261\t0.000000\n"
    "5262\t0.000000\n5263\t0.000000\n5264\t0.000000\n5265\t0.000000\n"
    "5266\t0.000000\n5281\t0.000000\n5282\t0.000000\n5283\t0.000000\n"
    "5284\t0.000000\n5285\t0.000000\n5286\t0.000000\n5301\t0.000000\n"
    "5302\t0.000000\n5303\t0.000000\n5304\t0.000000\n5305\t0.000000\n"
    "5306\t0.000000\n5321\t0.000000\n5322\t0.000000\n5323\t0.000000\n"
    "5324\t0.000000\n5325\t0.000000\n5326\t0.000000\n5341\t0.000000\n"
    "5342\t0.000000\n5343\t0.000000\n5344\t0.000000\n5345\t0.000000\n"
    "5346\t0.000000\n5361\t0.000000\n5362\t0.000000\n5363\t0.000000\n"
    "5364\t0.000000\n5365\t0.000000\n5366\t0.000000\n5381\t0.000000\n"
    "5382\t0.000000\n5383\t0.000000\n5384\t0.000000\n5385\t0.000000\n"
    "5386\t0.000000\n";

/// One tool in slot 1 so that a file with a T1 M6 in it interprets. Kept
/// deliberately small; a real tool table comes from the settings dialog.
struct DefaultTool { int slot; int id; double length; double diameter; };
const DefaultTool kDefaultTools[] = {
    { 1, 1, 0.0, 0.125 },
    { 2, 2, 0.0, 0.250 },
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

std::string errorTextFor(int errorCode)
{
    char buffer[RS274NGC_TEXT_SIZE];
    std::string text;

    rs274ngc_error_text(errorCode, buffer, RS274NGC_TEXT_SIZE);
    text = (buffer[0] == 0) ? "Unknown error, bad error code" : buffer;

    rs274ngc_line_text(buffer, RS274NGC_TEXT_SIZE);
    if (buffer[0] != 0) {
        text += "\n";
        text += buffer;
    }

    for (int k = 0; ; k++) {
        rs274ngc_stack_name(k, buffer, RS274NGC_TEXT_SIZE);
        if (buffer[0] == 0)
            break;
        text += "\n  in ";
        text += buffer;
    }
    return text;
}

/// Load the built-in tool table into _tools.
void loadDefaultTools()
{
    for (int slot = 0; slot <= _tool_max; slot++) {
        _tools[slot].id = -1;
        _tools[slot].length = 0;
        _tools[slot].diameter = 0;
    }
    for (size_t i = 0; i < sizeof(kDefaultTools) / sizeof(kDefaultTools[0]); i++) {
        const DefaultTool &t = kDefaultTools[i];
        if (t.slot < 0 || t.slot > _tool_max)
            continue;
        _tools[t.slot].id = t.id;
        _tools[t.slot].length = t.length;
        _tools[t.slot].diameter = t.diameter;
    }
}

/// Pull the value that follows \a letter out of a LinuxCNC style tool line,
/// e.g. 'D' out of "T1 P1 Z0.0 D0.125". The caller has already cut off any
/// comment, so a 'T' in a tool description cannot be mistaken for a tool word.
/// Returns false when the letter is absent or is not followed by a number.
bool toolWord(const std::string &line, char letter, double *value)
{
    for (size_t i = 0; i < line.size(); i++) {
        if (toupper((unsigned char)line[i]) != letter)
            continue;
        // must start a word, not sit inside one
        if (i > 0 && !isspace((unsigned char)line[i - 1]))
            continue;
        const char *start = line.c_str() + i + 1;
        char *end = NULL;
        double v = strtod(start, &end);
        if (end == start)
            continue;
        *value = v;
        return true;
    }
    return false;
}

/// Read a tool table into _tools.
///
/// Accepts both the interpreter's own format (free-form header, a blank line,
/// then "slot id tool-length-offset diameter" per line) and the LinuxCNC style
/// "T1 P1 Z0.0 D0.125 ; comment" that qgcoder itself writes. Anything from a
/// ';' or '#' onwards is a comment, and lines that are then blank, or that
/// parse as neither format, are skipped -- which is how the header lines of
/// both formats get past.
///
/// Returns false, with \a error set, only when the file cannot be opened or
/// contains no usable tool at all.
bool readToolFile(const std::string &fileName, std::string *error)
{
    FILE *toolFile = fopen(fileName.c_str(), "r");
    if (toolFile == NULL) {
        *error = "Cannot open tool file " + fileName;
        return false;
    }

    for (int slot = 0; slot <= _tool_max; slot++) {
        _tools[slot].id = -1;
        _tools[slot].length = 0;
        _tools[slot].diameter = 0;
    }

    char buffer[1000];
    int toolsRead = 0;
    while (fgets(buffer, sizeof(buffer), toolFile) != NULL) {
        std::string line(buffer);
        size_t comment = line.find_first_of(";#");
        if (comment != std::string::npos)
            line.erase(comment);
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            continue;

        int slot = 0;
        int id = 0;
        double length = 0;
        double diameter = 0;

        if (line[first] == 'T' || line[first] == 't') {
            double v;
            if (!toolWord(line, 'T', &v))
                continue;                         // not a tool line after all
            id = (int)v;
            slot = toolWord(line, 'P', &v) ? (int)v : id;
            length = toolWord(line, 'Z', &v) ? v : 0.0;
            diameter = toolWord(line, 'D', &v) ? v : 0.0;
        } else if (sscanf(line.c_str(), "%d %d %lf %lf",
                          &slot, &id, &length, &diameter) == 4) {
            // interpreter's own format
        } else {
            continue;                             // header or comment line
        }

        if (slot < 0 || slot > _tool_max) {
            fclose(toolFile);
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "Out of range tool slot number %d in tool file ", slot);
            *error = std::string(msg) + fileName;
            return false;
        }
        _tools[slot].id = id;
        _tools[slot].length = length;
        _tools[slot].diameter = diameter;
        toolsRead++;
    }
    fclose(toolFile);

    if (toolsRead == 0) {
        *error = "No tools found in tool file " + fileName;
        return false;
    }
    return true;
}

/// Make sure \a path exists and holds a usable parameter file, creating it from
/// the built-in defaults when it does not.
bool ensureParameterFile(const std::string &path, std::string *error)
{
    FILE *existing = fopen(path.c_str(), "r");
    if (existing != NULL) {
        fclose(existing);
        return true;
    }

    FILE *created = fopen(path.c_str(), "w");
    if (created == NULL) {
        *error = "Cannot create parameter file " + path;
        return false;
    }
    fputs(kDefaultParameters, created);
    fclose(created);
    return true;
}

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
    return dir + RS274NGC_PARAMETER_FILE_NAME_DEFAULT;
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

    const std::string varFile =
        parameterFile.empty() ? defaultParameterFile() : parameterFile;
    if (!ensureParameterFile(varFile, &result.error))
        return result;
    if (varFile.size() >= RS274NGC_TEXT_SIZE) {   // _parameter_file_name's size
        result.error = "Parameter file path is too long: " + varFile;
        return result;
    }
    strcpy(_parameter_file_name, varFile.c_str());

    if (toolTable.empty()) {
        loadDefaultTools();
    } else if (!readToolFile(toolTable, &result.error)) {
        return result;
    }

    // The canon layer prints through _outfile. An in-memory stream lets us pick
    // complete lines out of it as they appear, which is what reading the
    // interpreter's stdout used to do.
    char *buffer = NULL;
    size_t length = 0;
    FILE *stream = open_memstream(&buffer, &length);
    if (stream == NULL) {
        result.error = "Cannot open the interpreter output stream";
        return result;
    }
    _outfile = stream;

    size_t consumed = 0;
    int status = rs274ngc_init();
    if (status != RS274NGC_OK) {
        result.error = errorTextFor(status);
        fclose(stream);
        free(buffer);
        _outfile = NULL;
        return result;
    }

    status = rs274ngc_open(ngcFile.c_str());
    if (status != RS274NGC_OK) {
        result.error = errorTextFor(status);
        rs274ngc_exit();
        fclose(stream);
        free(buffer);
        _outfile = NULL;
        return result;
    }

    // This loop is interpret_from_file() from driver.cc with do_next == 2
    // ("stop on error"), minus the MDI prompts that needed a terminal.
    bool failed = false;
    for (;;) {
        if (shouldAbort && shouldAbort()) {
            result.aborted = true;
            break;
        }

        status = rs274ngc_read(NULL);
        if (status == RS274NGC_ENDFILE)
            break;
        if (status != RS274NGC_OK && status != RS274NGC_EXECUTE_FINISH) {
            result.error = errorTextFor(status);
            failed = true;
            break;
        }

        status = rs274ngc_execute();
        fflush(stream);
        result.canonLines += drain(buffer, length, &consumed, onLine);

        if (status == RS274NGC_EXIT) {
            result.reachedEnd = true;
            break;
        }
        if (status != RS274NGC_OK && status != RS274NGC_EXECUTE_FINISH) {
            result.error = errorTextFor(status);
            failed = true;
            break;
        }
    }

    rs274ngc_close();
    rs274ngc_exit();                              /* saves the parameters */

    fflush(stream);
    result.canonLines += drain(buffer, length, &consumed, onLine);
    if (consumed < length)                        // a final line with no newline
        onLine(std::string(buffer + consumed, length - consumed));

    fclose(stream);
    free(buffer);
    _outfile = NULL;

    result.ok = !failed;
    return result;
}

} // namespace rs274ngc
