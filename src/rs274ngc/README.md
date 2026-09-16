# rs274ngc — vendored G-code interpreter

The NIST RS274NGC interpreter, built straight into `qgcoder`. Until now qgcoder
shelled out to a separate `rs274` executable and drove it over a pipe, feeding
its interactive menu (`3`, tool table path, `1`) on stdin and reading canonical
commands back from stdout. That executable is gone: `rs274ngc_interp.{hpp,cpp}`
calls the same interpreter in-process instead.

## Provenance

Copied from <https://github.com/QGCoder/rs274ngc> at commit `c895314`
("lintian removed for now.", 2021-12-26), itself a mirror of the original
[NIST RS274NGC interpreter](https://code.google.com/archive/p/rs274ngc/).
Licensed GPL-3.0-or-later; see [LICENSE](LICENSE).

| File | Origin |
| --- | --- |
| `canon_pre.cc`, `rs274ngc_pre.cc` | upstream, see local changes below |
| `canon.hh`, `rs274ngc.hh`, `rs274ngc_errors.cc`, `rs274ngc_return.hh` | upstream, unchanged |
| `rs274ngc.var`, `rs274ngc.tool_default` | upstream, unchanged — reference copies only, see below |
| `rs274ngc_interp.hpp`, `rs274ngc_interp.cpp` | qgcoder, replaces upstream `driver.cc` |

`rs274ngc_pre.cc` `#include`s `rs274ngc_errors.cc`, so that file must not be
compiled on its own.

Upstream `driver.cc` is deliberately **not** vendored. It is a `main()` that
prompts on stderr, reads menu choices from stdin, and calls `exit()` on every
error — none of which suits a library. `rs274ngc_interp.cpp` does the same work
as a callable function that returns errors instead of exiting, and hands each
canonical command to a callback instead of printing it to stdout.

`rs274ngc.var` and `rs274ngc.tool_default` are kept as reference copies of the
two file formats; nothing reads them at build or run time. The built-in defaults
compiled into `rs274ngc_interp.cpp` are derived from them.

## Local changes to the upstream sources

All are marked with a `qgcoder:` comment. Two are in `canon_pre.cc`:

1. **`INIT_CANON()` resets the dummy world model.** Upstream leaves it empty,
   which is fine when every interpretation is a fresh process. In-process, the
   file-scope statics (`_line_number`, `_length_unit_type`, `_program_position_*`,
   `_active_slot`, …) would otherwise survive into the next run, so a second
   preview would start in the units, at the position, and with the tool the
   previous one happened to end with. The reset reproduces the start-up values
   exactly, including the three that upstream leaves zero-initialised.

2. **`_parameter_file_name` grew from `[100]` to `[256]`.** qgcoder keeps the
   parameter file under `QStandardPaths::AppDataLocation`, whose absolute path
   can exceed 99 characters. 256 is `RS274NGC_TEXT_SIZE`, the size of every
   buffer the name is copied into, and `GET_EXTERNAL_PARAMETER_FILE_NAME` still
   refuses to overrun a shorter destination.

and one is in `rs274ngc_pre.cc`:

3. **`close_and_downcase()` treats `;` as a comment to end of line.** RS274NGC
   only knows parenthesised comments, so a `;` was a `NCE_BAD_CHARACTER_USED`
   error. Plenty of generators emit `;` comments — `hf2gcode`, which qgcoder
   ships as its default command, puts one on every glyph — and the whole file
   would fail to interpret. The rest of the line is dropped, leaving a block no
   different from a blank line, which is what LinuxCNC does. A `;` *inside* a
   `(...)` comment is untouched. The text is dropped rather than rewritten into
   a `(...)` comment on purpose: `; rapid to (0,0)` would otherwise raise
   `NCE_NESTED_COMMENT_FOUND`.

## Notes for callers

* **One run at a time.** The interpreter keeps all of its state in file-scope
  globals. `Interpreter::interpretFile()` serialises callers on an internal
  mutex; it is safe to call from a worker thread, but two interpretations never
  overlap.
* **Locale.** The interpreter reads and writes every coordinate with
  `strtod()`/`fprintf()`, which follow `LC_NUMERIC`. As a separate process it
  always ran in the `"C"` locale; linked into a Qt application it would inherit
  whatever Qt set, and in a locale that writes `1,5` for one-and-a-half every
  fractional coordinate would be silently truncated to zero.
  `interpretFile()` pins `LC_NUMERIC` to `"C"` for the duration of the run with
  `uselocale()`, which affects only the calling thread.
* **Tool tables.** `readToolFile()` accepts both the interpreter's own format
  (header, blank line, then `slot id tool-length-offset diameter`) and the
  LinuxCNC style `T1 P1 Z0.0 D0.125` that qgcoder itself writes. An unset or
  missing tool table is not an error — the built-in default table is used.
* **Parameter file.** Created from the built-in default when missing, and
  rewritten by `rs274ngc_exit()` at the end of every run, exactly as the
  stand-alone executable did.

## Updating from upstream

Re-copy the files listed as "upstream" above, then re-apply the three changes
listed under "Local changes". `rs274ngc_interp.cpp` only uses the public
`rs274ngc_*` API declared in `rs274ngc.hh`, plus the three driver-owned globals
`_tools`, `_tool_max` and `_parameter_file_name`.
