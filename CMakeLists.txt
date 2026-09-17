cmake_minimum_required(VERSION 3.21)
project(qgcoder VERSION 0.1.43 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# A Mac building from source would otherwise default CMAKE_OSX_DEPLOYMENT_TARGET
# to the macOS it is running on, silently raising the floor for people on
# older releases. Keep it at 11.0, the oldest macOS qgcoder supports (Big Sur).
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0 CACHE STRING "Minimum macOS version" FORCE)
endif()

# Qt 6.7 - the newest line that still runs on macOS 11 - bundles a
# FindWrapOpenGL.cmake that falls back to linking the obsolete Carbon-era AGL
# framework when it cannot find it, and AGL is gone from the newer macOS SDKs.
# Nothing here draws through AGL: the view is a QOpenGLWidget on the 3.3 core
# profile. Make Qt's module find its target already defined, so it leaves its
# own AGL fallback alone.
if(APPLE)
    add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)
    set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES
        INTERFACE_LINK_LIBRARIES "-framework OpenGL")
endif()

# 6.4 is what Ubuntu 24.04 LTS ships, and it is the first release with the
# Qt::StringLiterals _s / _L1 operators this code uses.
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
qt_standard_project_setup()

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src)

# qt_standard_project_setup() turns on AUTOMOC and AUTOUIC but not AUTORCC, so
# without this res.qrc is listed as a source below and then quietly ignored -
# taking the window icon with it.
set(CMAKE_AUTORCC ON)

# The RS274NGC interpreter, vendored from LinuxCNC under src/linuxcnc/ and
# linked straight into qgcoder - there is no separate "rs274" executable to
# run any more. See src/linuxcnc/README.md.
add_subdirectory(src/linuxcnc)

# The G-code editor widget: https://github.com/QGCoder/libqgcodeeditor
find_library(QGCODEEDITOR_LIBRARY NAMES qgcodeeditor_qt6 qgcodeeditor REQUIRED)
find_path(QGCODEEDITOR_INCLUDE_DIR
    NAMES QGCodeEditor/QGCodeEditor.h
    HINTS ${Qt6Core_INCLUDE_DIRS}
    PATHS /usr/include /usr/local/include
    PATH_SUFFIXES qt6
    REQUIRED
)

qt_add_executable(qgcoder
    src/main.cpp
    src/mainwin.cpp src/mainwin.h src/mainwin.ui
    src/view.cpp src/view.h
    src/lex_analyzer.cpp src/lex_analyzer.hpp
    src/settings_dlg.cpp src/settings_dlg.h src/settings.ui
    src/g2m/canonLine.cpp src/g2m/canonLine.hpp
    src/g2m/canonMotion.cpp src/g2m/canonMotion.hpp
    src/g2m/canonMotionless.cpp src/g2m/canonMotionless.hpp
    src/g2m/g2m.cpp src/g2m/g2m.hpp
    src/g2m/g2mworker.cpp src/g2m/g2mworker.h
    src/g2m/gplayer.hpp
    src/g2m/helicalMotion.cpp src/g2m/helicalMotion.hpp
    src/g2m/linearMotion.cpp src/g2m/linearMotion.hpp
    src/g2m/machineStatus.cpp src/g2m/machineStatus.hpp
    src/g2m/nanotimer.cpp src/g2m/nanotimer.hpp
    src/g2m/point.hpp
    src/res.qrc
)

target_include_directories(qgcoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/g2m
    ${QGCODEEDITOR_INCLUDE_DIR}
)

if(MSVC)
    target_compile_options(qgcoder PRIVATE /W3 /permissive-)
else()
    target_compile_options(qgcoder PRIVATE
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Debug>:-O0>
        -Wall
        -Wno-deprecated-copy
    )
endif()

target_compile_definitions(qgcoder PRIVATE
    QT_DISABLE_DEPRECATED_UP_TO=0x060400
    QT_NO_CAST_TO_ASCII
    # g2m uses M_PI, which the Windows CRT headers only declare when asked
    _USE_MATH_DEFINES
)

target_link_libraries(qgcoder PRIVATE
    linuxcnc-rs274ngc
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    ${QGCODEEDITOR_LIBRARY}
)

# This is a GUI application: no console window behind it on Windows, and a
# .app bundle rather than a bare executable on macOS. qt_add_executable()
# passes its arguments straight to add_executable() and sets neither.
set_target_properties(qgcoder PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

if(APPLE)
    set_target_properties(qgcoder PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "QGCoder"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.qgcoder"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
    # icons/qgcoder.icns is generated from the PNG by the release workflow; a
    # plain source build just gets the default application icon.
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/icons/qgcoder.icns)
        target_sources(qgcoder PRIVATE src/icons/qgcoder.icns)
        set_source_files_properties(src/icons/qgcoder.icns PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources)
        set_target_properties(qgcoder PROPERTIES
            MACOSX_BUNDLE_ICON_FILE qgcoder.icns)
    endif()
endif()

install(TARGETS qgcoder
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

# Emscripten: the build produces qgcoder.html, qgcoder.js and qgcoder.wasm, and
# the page pulls in qtloader.js and the logo shown while it loads, both of which
# Qt ships next to the platform plugin. Install the lot side by side so the
# install prefix is a directory a web server can be pointed straight at.
if(EMSCRIPTEN)
    install(FILES
        $<TARGET_FILE_DIR:qgcoder>/qgcoder.html
        $<TARGET_FILE_DIR:qgcoder>/qgcoder.js
        $<TARGET_FILE_DIR:qgcoder>/qgcoder.wasm
        $<TARGET_FILE_DIR:qgcoder>/qtloader.js
        $<TARGET_FILE_DIR:qgcoder>/qtlogo.svg
        DESTINATION bin
    )
endif()

# the freedesktop entry and icon only mean anything on a freedesktop system
if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    install(FILES qgcoder.desktop DESTINATION share/applications)
    install(FILES src/icons/qgcoder.png DESTINATION share/icons/hicolor/256x256/apps)
endif()

# Windows and macOS have no package manager to pull Qt in, so `cmake --install`
# there also runs windeployqt / macdeployqt and leaves a self-contained app.
# Linux keeps the plain install: the .deb depends on the system Qt.
if(WIN32 OR APPLE)
    qt_generate_deploy_app_script(
        TARGET qgcoder
        OUTPUT_SCRIPT qgcoder_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${qgcoder_deploy_script})
endif()

# ---------------------------------------------------------------------------
# Tests
#
# The g-code programs in src/tests exercise what the LinuxCNC interpreter does
# and the NIST one it replaced could not: o-word subroutines with parameters,
# loops and conditionals, and the canned cycles. Each is interpreted and its
# canonical output compared against a recorded copy, so a change in what
# qgcoder would draw shows up as a diff rather than as a bug report.
#
# They need a host binary, so they are skipped when cross-compiling - which is
# what the WebAssembly build is.
# ---------------------------------------------------------------------------
if(NOT CMAKE_CROSSCOMPILING)
    enable_testing()

    add_executable(interp_test src/tests/interp_test.cpp)
    set_target_properties(interp_test PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    target_link_libraries(interp_test PRIVATE linuxcnc-rs274ngc)

    foreach(case oword cycles)
        add_test(NAME interp-${case}
                 COMMAND ${CMAKE_COMMAND}
                         -DTOOL=$<TARGET_FILE:interp_test>
                         -DNGC=${CMAKE_CURRENT_SOURCE_DIR}/src/tests/${case}.ngc
                         -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/src/tests/${case}.expected
                         -DWORKDIR=${CMAKE_CURRENT_BINARY_DIR}/tests/${case}
                         -P ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/run_case.cmake)
    endforeach()
endif()
