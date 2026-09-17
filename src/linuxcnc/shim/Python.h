#pragma once
/* Python-free build: enough of the CPython API for the interpreter's dead
 * Python paths to compile. Nothing here is ever reached - python_plugin is
 * always NULL, so PYUSABLE is false everywhere these appear. */
struct _typeobject { const char *tp_name; };
struct _object { int ob_refcnt; struct _typeobject *ob_type; };
typedef struct _object PyObject;
typedef struct _ts PyThreadState;
extern PyObject *Py_None;
inline int  Py_IsInitialized(void) { return 0; }
inline void PyErr_Print(void) {}
inline void PyErr_Clear(void) {}
inline PyObject *PyErr_Occurred(void) { return nullptr; }
inline void PyErr_Fetch(PyObject **a, PyObject **b, PyObject **c) { *a = *b = *c = nullptr; }
inline void PyErr_NormalizeException(PyObject **, PyObject **, PyObject **) {}
inline PyObject *PyObject_Str(PyObject *) { return nullptr; }
inline int PyUnicode_Check(PyObject *) { return 0; }
inline int PyLong_Check(PyObject *) { return 0; }
inline int PyFloat_Check(PyObject *) { return 0; }
inline int PyBool_Check(PyObject *) { return 0; }
inline int PyObject_IsTrue(PyObject *) { return 0; }
inline const char *PyUnicode_AsUTF8(PyObject *) { return ""; }
inline long PyLong_AsLong(PyObject *) { return 0; }
inline double PyFloat_AsDouble(PyObject *) { return 0.0; }
inline void Py_XDECREF(PyObject *) {}
inline void Py_DECREF(PyObject *) {}
inline void Py_INCREF(PyObject *) {}
inline int PyCallable_Check(PyObject *) { return 0; }
extern PyObject *PyExc_KeyError;
extern PyObject *PyExc_TypeError;
extern PyObject *PyExc_ValueError;
inline void PyErr_SetString(PyObject *, const char *) {}
inline int PyErr_ExceptionMatches(PyObject *) { return 0; }
