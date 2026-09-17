#pragma once
/* Python-free build: boost::python is a compile-time dependency of the
 * interpreter (setup_struct has a boost::python::object member, and the remap
 * and named-parameter code names the types), but never a runtime one - every
 * use sits behind PYUSABLE, which is false when python_plugin is NULL, and it
 * always is here. These types exist so the upstream sources compile
 * unmodified; reaching one throws. */
#include <Python.h>
#include <stdexcept>
namespace boost {
template <class T> const T &cref(const T &t) { return t; }
namespace python {

struct error_already_set : std::runtime_error {
    error_already_set() : std::runtime_error("built without Python support") {}
};

class object {
  public:
    object() {}
    template <class T> object(const T &) {}
    object attr(const char *) const { return object(); }
    object operator[](const char *) const { return object(); }
    template <class T> object operator[](const T &) const { return object(); }
    object operator[](int) const { return object(); }
    object &operator=(const object &) { return *this; }
    explicit operator bool() const { return false; }
    PyObject *ptr() const { return nullptr; }
};

class list : public object {
  public:
    template <class T> void append(const T &) {}
};
inline long len(const object &) { return 0; }
class dict : public object {
  public:
    list keys() const;
    object values() const { return object(); }
    bool has_key(const char *) const { return false; }
};
class tuple : public object {
  public:
    tuple() {}
    explicit tuple(const list &) {}
};

template <class T> struct extract {
    template <class U> extract(const U &) {}
    operator T() const { throw error_already_set(); }
    bool check() const { return false; }
    T operator()() const { throw error_already_set(); }
};

inline object import(const char *) { throw error_already_set(); }
inline void handle_exception() {}

struct scope {
    template <class T> scope(const T &) {}
    object attr(const char *) { return object(); }
};

inline list dict::keys() const { return list(); }

} }
