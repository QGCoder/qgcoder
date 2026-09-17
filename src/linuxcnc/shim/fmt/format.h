#pragma once
/// \file
/// A stand-in for the {fmt} library, covering the subset LinuxCNC's inifile.cc
/// uses: plain "{}" substitution and zero-padded hex such as "{:04X}".
///
/// Vendoring real {fmt} would add a dependency to every platform qgcoder
/// builds on, including WebAssembly, for what amounts to a handful of error
/// strings. std::format would do, but its support is still uneven across the
/// compilers in use here.
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

namespace fmt {
namespace detail {

inline void append(std::ostringstream &out, const std::string &spec, const char *text)
{
    (void)spec;
    out << text;
}

template <class T>
void append(std::ostringstream &out, const std::string &spec, const T &value)
{
    // spec is either empty or of the form "0<width><type>", e.g. "04X"
    if (spec.empty()) {
        out << value;
        return;
    }

    std::ostringstream field;
    std::size_t i = 0;
    char pad = ' ';
    if (spec[i] == '0') { pad = '0'; ++i; }

    int width = 0;
    while (i < spec.size() && spec[i] >= '0' && spec[i] <= '9')
        width = width * 10 + (spec[i++] - '0');

    const char type = (i < spec.size()) ? spec[i] : 'd';
    if (type == 'X' || type == 'x') {
        field << std::hex << (type == 'X' ? std::uppercase : std::nouppercase);
    }
    field << std::setfill(pad) << std::setw(width) << value;
    out << field.str();
}

inline void format_to(std::ostringstream &out, std::string_view f)
{
    out << f;
}

template <class T, class... Rest>
void format_to(std::ostringstream &out, std::string_view f, const T &value, const Rest &...rest)
{
    const std::size_t open = f.find('{');
    if (open == std::string_view::npos) {
        out << f;
        return;
    }
    out << f.substr(0, open);

    const std::size_t close = f.find('}', open);
    if (close == std::string_view::npos) {
        out << f.substr(open);
        return;
    }

    std::string spec(f.substr(open + 1, close - open - 1));
    const std::size_t colon = spec.find(':');
    spec = (colon == std::string::npos) ? std::string() : spec.substr(colon + 1);

    append(out, spec, value);
    format_to(out, f.substr(close + 1), rest...);
}

} // namespace detail

template <class... Args>
std::string format(std::string_view f, const Args &...args)
{
    std::ostringstream out;
    detail::format_to(out, f, args...);
    return out.str();
}

} // namespace fmt
