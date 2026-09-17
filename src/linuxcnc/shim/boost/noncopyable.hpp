#pragma once
/// \file
/// boost::noncopyable, which InterpBase derives from.
///
/// This is the whole of what upstream uses it for, and carrying it here means
/// the build does not need Boost installed. Without it the vendored sources
/// compile on a machine that happens to have libboost-dev and nowhere else.
namespace boost {

/// Deriving from this deletes the copy constructor and copy assignment.
class noncopyable {
  protected:
    noncopyable() = default;
    ~noncopyable() = default;
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;
};

} // namespace boost
