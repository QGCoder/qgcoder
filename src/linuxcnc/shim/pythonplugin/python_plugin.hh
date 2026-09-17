#pragma once
/* Python-free build: the plugin never exists, so PYUSABLE is always false and
 * every Python path in the interpreter is unreachable. */
#include <boost/python/object.hpp>
#include <string>
#define PLUGIN_OK 0
#define PLUGIN_EXCEPTION 1
class PythonPlugin {
  public:
    bool usable() const { return false; }
    int  plugin_status() const { return PLUGIN_EXCEPTION; }
    std::string last_exception() const { return "built without Python support"; }
    template <class... A> int call(A &&...) { return PLUGIN_EXCEPTION; }
    template <class... A> int call_method(A &&...) { return PLUGIN_EXCEPTION; }
    template <class... A> int callable(A &&...) { return 0; }
    template <class... A> bool is_callable(A &&...) { return false; }
    template <class... A> int configure(A &&...) { return PLUGIN_EXCEPTION; }
    boost::python::dict main_namespace;
    template <class... A> int run_string(A &&...) { return PLUGIN_EXCEPTION; }
    /// The Interp constructor early-returns if this yields nothing, leaving a
    /// half-built object, so hand back a plugin that simply is not usable.
    /// The python_plugin global stays NULL, which is what PYUSABLE tests.
    static PythonPlugin *instantiate(...)
    {
        static PythonPlugin unusable;
        return &unusable;
    }
};
extern PythonPlugin *python_plugin;

/// The interpreter calls this where it would report a Python traceback.
inline std::string handle_pyerror() { return "built without Python support"; }
