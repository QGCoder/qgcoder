# LinuxCNC RS274NGC interpreter, embedded

The G-code interpreter from [LinuxCNC](https://github.com/LinuxCNC/linuxcnc),
vendored so that qgcoder can interpret the same dialect LinuxCNC runs, on every
platform qgcoder builds for — including Windows, macOS and WebAssembly, none of
which have LinuxCNC packages.

Upstream revision: `bae3bfbdfb97e9bea4295afc6079e1c74a3e0958`

## What was taken

`emc/rs274ngc/` is the interpreter and `emc/rs274ngc/saicanon.cc` the canon
layer from `emc/sai/`, the one LinuxCNC's own standalone `rs274` tool uses —
it prints canonical commands as text, which is exactly what qgcoder's g2m
parser reads. The rest (`emc/nml_intf`, `emc/ini`, `emc/tooldata`,
`libnml`, `libposemath`, `rtapi`, `hal`) is the header and helper closure
those need, computed with `g++ -MM` rather than guessed at.

**The upstream sources are unmodified.** Everything qgcoder needs to change to
build them lives in `shim/` and `nopython.cc`, so moving to a newer LinuxCNC
is a matter of copying files over the top.

## What was left out, and how

No Python. LinuxCNC's interpreter can call Python for O-word subroutines,
remapped codes and named parameters; qgcoder previews tool paths and has no use
for any of it, and Python would have to be built for four platforms to get
there.

Python cannot simply be compiled out — there is no build switch for it, and
`setup_struct` holds a `boost::python::object` member, so the types have to
exist. Instead:

* `shim/boost/python/` and `shim/Python.h` declare just enough of both APIs
  for the upstream sources to compile. Every function throws or returns a
  failure; none is ever called.
* `shim/pythonplugin/python_plugin.hh` provides a plugin that reports itself
  unusable. `PYUSABLE` is `python_plugin != NULL && python_plugin->usable()`,
  and the global stays `NULL`, so every Python path in the interpreter is dead
  code.
* `nopython.cc` defines the handful of entry points that lived in
  `interp_python.cc`, which is not vendored.

`shim/fmt/format.h` is a stand-in for the {fmt} library, which upstream uses
for error strings in `inifile.cc` alone. It covers the subset used there,
rather than adding a dependency to every platform.

HAL is stubbed the same way: `#<_hal[...]>` parameters need a running machine,
and there is not one here.

## Licence

LinuxCNC is GPL-2; qgcoder is GPL-2-or-later. See LICENSE.
