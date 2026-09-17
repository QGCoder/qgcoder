/// \file
/// The parts of LinuxCNC's interpreter that only exist to talk to Python, and
/// the HAL accessors that only mean something with a machine attached.
///
/// qgcoder embeds the interpreter to preview a tool path, so neither is
/// reachable: python_plugin is NULL, which makes PYUSABLE false at every call
/// site, and there is no HAL to query. These definitions exist so the upstream
/// sources link unmodified.

#include "rs274ngc.hh"
#include "rs274ngc_interp.hh"
#include "pythonplugin/python_plugin.hh"
#include "interp_base.hh"
#include "interp_internal.hh"
#include "interp_return.hh"
#include "tooldata.hh"

/// Not NULL: the interpreter calls python_plugin->is_callable() without
/// checking in a couple of places. PYUSABLE is
/// "python_plugin && python_plugin->usable()", and usable() is false, so every
/// Python path stays dead either way - this one just has no null dereference
/// waiting in it.
static PythonPlugin g_unusablePlugin;
PythonPlugin *python_plugin = &g_unusablePlugin;

/// #<_task> tells a g-code program whether it is running under LinuxCNC's task
/// controller or only being previewed. It is a preview here, always.
int _task = 0;

/// The interpreter the canon layer reads the current g-code line from. The
/// driver points this at its own instance before interpreting; saicanon
/// dereferences it without checking.
InterpBase *pinterp = nullptr;

/// No tool database process to ask; the tool table comes from a file.
int tooldata_db_getall(void) { return -1; }

PyObject *Py_None = nullptr;
PyObject *PyExc_KeyError = nullptr;
PyObject *PyExc_TypeError = nullptr;
PyObject *PyExc_ValueError = nullptr;

pycontext::pycontext() {}
pycontext::~pycontext() {}

int Interp::py_reload() { return INTERP_OK; }
int Interp::py_execute(const char * /*cmd*/, bool /*as_file*/) { return INTERP_OK; }
bool Interp::is_pycallable(setup * /*s*/, const char * /*module*/, const char * /*funcname*/)
{
    return false;
}
int Interp::pycall(setup * /*s*/, context_struct * /*frame*/, const char * /*module*/,
                   const char * /*funcname*/, int /*calltype*/)
{
    return INTERP_ERROR;
}

// HAL pin access behind #<_hal[...]>; there is no HAL here.
extern "C" {
int hal_is_init(void) { return 0; }
int hal_get_p(const char * /*name*/, char * /*buf*/, int /*size*/) { return -1; }
int hal_get_s(const char * /*name*/, char * /*buf*/, int /*size*/) { return -1; }
}

// ---------------------------------------------------------------------------
// Emscripten declares wordexp() in <wordexp.h> but does not implement it, so
// the interpreter compiles and then fails to link. rs274ngc_pre.cc uses it to
// expand shell syntax in directory names read from a machine .ini, which
// qgcoder never supplies, so these hand the word back unexpanded - valid, one
// word, and never reached. Windows takes the header in shim/win instead,
// because there it does not exist at all.
// ---------------------------------------------------------------------------
#ifdef __EMSCRIPTEN__
#include <wordexp.h>
#include <cstdlib>
#include <cstring>

extern "C" {

/// \param words the text to expand
/// \param result filled in with \a words as a single word
/// \param flags ignored
/// \returns 0, or WRDE_NOSPACE if it could not allocate
int wordexp(const char *words, wordexp_t *result, int flags)
{
    (void)flags;
    if (!result)
        return WRDE_NOSPACE;
    result->we_wordv = (char **)calloc(2, sizeof(char *));
    if (!result->we_wordv)
        return WRDE_NOSPACE;
    result->we_wordv[0] = strdup(words ? words : "");
    result->we_wordv[1] = nullptr;
    result->we_wordc = 1;
    result->we_offs = 0;
    return 0;
}

/// \param result the result to release
void wordfree(wordexp_t *result)
{
    if (!result || !result->we_wordv)
        return;
    for (size_t i = 0; i < result->we_wordc; ++i)
        free(result->we_wordv[i]);
    free(result->we_wordv);
    result->we_wordv = nullptr;
    result->we_wordc = 0;
}

} // extern "C"
#endif // __EMSCRIPTEN__
