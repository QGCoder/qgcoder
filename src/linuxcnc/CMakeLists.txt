# The LinuxCNC RS274NGC interpreter, built as a static library.
#
# The upstream sources are vendored unmodified (see README.md), so everything
# needed to compile them out of their own build system is applied here rather
# than by patching them.

file(GLOB LINUXCNC_INTERP_SOURCES CONFIGURE_DEPENDS emc/rs274ngc/*.cc)
add_library(linuxcnc-rs274ngc STATIC
    ${LINUXCNC_INTERP_SOURCES}
    nopython.cc
    emc/ini/inifile.cc
    emc/nml_intf/modal_state.cc
    emc/tooldata/tooldata_common.cc
    emc/tooldata/tooldata_nml.cc
)

set_target_properties(linuxcnc-rs274ngc PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    POSITION_INDEPENDENT_CODE ON
)

# shim/ first: it supplies boost/python, Python.h and fmt/format.h, and must
# win over any real ones that happen to be installed.
target_include_directories(linuxcnc-rs274ngc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/shim
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/emc
    ${CMAKE_CURRENT_SOURCE_DIR}/emc/rs274ngc
    ${CMAKE_CURRENT_SOURCE_DIR}/emc/nml_intf
    ${CMAKE_CURRENT_SOURCE_DIR}/emc/tooldata
    ${CMAKE_CURRENT_SOURCE_DIR}/emc/kinematics
    ${CMAKE_CURRENT_SOURCE_DIR}/emc/ini
    ${CMAKE_CURRENT_SOURCE_DIR}/libnml
    ${CMAKE_CURRENT_SOURCE_DIR}/libnml/nml
    ${CMAKE_CURRENT_SOURCE_DIR}/libposemath
    ${CMAKE_CURRENT_SOURCE_DIR}/rtapi
    ${CMAKE_CURRENT_SOURCE_DIR}/hal
)

target_compile_definitions(linuxcnc-rs274ngc PUBLIC
    ULAPI               # rtapi builds for userspace, not the realtime kernel
    UNIT_TEST           # upstream's own guard around the Python 'this' export
    _USE_MATH_DEFINES   # M_PI, which the Windows CRT headers hide by default
)

# Headers the upstream sources rely on having been pulled in by LinuxCNC's own
# build. Forced in rather than added to the sources, which stay untouched.
# The SHELL: prefix keeps each "-include <header>" pair together; without it
# CMake de-duplicates the repeated -include and the headers become input files.
if(MSVC)
    target_compile_options(linuxcnc-rs274ngc PRIVATE
        /FIcstring /FIcwctype /FIcmath /w)
else()
    target_compile_options(linuxcnc-rs274ngc PRIVATE
        "SHELL:-include cstring"
        "SHELL:-include strings.h"
        "SHELL:-include cwctype"
        "SHELL:-include cmath"
        -w)
endif()
