cmake_minimum_required(VERSION 3.21)
project(qgcoder VERSION 0.1.36 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 6.4 is what Ubuntu 24.04 LTS ships, and it is the first release with the
# Qt::StringLiterals _s / _L1 operators this code uses.
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
qt_standard_project_setup()

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR})

# The NIST RS274NGC interpreter, vendored under rs274ngc/ and linked straight
# into qgcoder - there is no separate "rs274" executable to run any more.
# The vendored files are 1990s C-with-classes, so they are built on their own
# with warnings off and without AUTOMOC: none of this is Qt.
add_library(rs274ngc STATIC
    rs274ngc/rs274ngc_pre.cc
    rs274ngc/canon_pre.cc
    rs274ngc/rs274ngc_interp.cpp
)
set_target_properties(rs274ngc PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories(rs274ngc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rs274ngc)
# only the vendored files get warnings turned off; rs274ngc_interp.cpp is ours
if(MSVC)
    set(rs274ngc_no_warnings "/w")
else()
    set(rs274ngc_no_warnings "-w")
endif()
set_source_files_properties(
    rs274ngc/rs274ngc_pre.cc
    rs274ngc/canon_pre.cc
    PROPERTIES COMPILE_OPTIONS "${rs274ngc_no_warnings}"
)

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
    main.cpp
    mainwin.cpp mainwin.h mainwin.ui
    view.cpp view.h
    lex_analyzer.cpp lex_analyzer.hpp
    settings_dlg.cpp settings_dlg.h settings.ui
    g2m/canonLine.cpp g2m/canonLine.hpp
    g2m/canonMotion.cpp g2m/canonMotion.hpp
    g2m/canonMotionless.cpp g2m/canonMotionless.hpp
    g2m/g2m.cpp g2m/g2m.hpp
    g2m/g2mworker.cpp g2m/g2mworker.h
    g2m/gplayer.hpp
    g2m/helicalMotion.cpp g2m/helicalMotion.hpp
    g2m/linearMotion.cpp g2m/linearMotion.hpp
    g2m/machineStatus.cpp g2m/machineStatus.hpp
    g2m/nanotimer.cpp g2m/nanotimer.hpp
    g2m/point.hpp
    res.qrc
)

target_include_directories(qgcoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/g2m
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
    rs274ngc
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
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/icons/qgcoder.icns)
        target_sources(qgcoder PRIVATE icons/qgcoder.icns)
        set_source_files_properties(icons/qgcoder.icns PROPERTIES
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
# the page pulls in qtloader.js, which Qt ships next to the platform plugin.
# Install the four of them side by side so the install prefix is a directory
# that can be served as-is.
if(EMSCRIPTEN)
    install(FILES
        $<TARGET_FILE_DIR:qgcoder>/qgcoder.html
        $<TARGET_FILE_DIR:qgcoder>/qgcoder.js
        $<TARGET_FILE_DIR:qgcoder>/qgcoder.wasm
        $<TARGET_FILE_DIR:qgcoder>/qtloader.js
        DESTINATION bin
    )
endif()

# the freedesktop entry and icon only mean anything on a freedesktop system
if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    install(FILES qgcoder.desktop DESTINATION share/applications)
    install(FILES icons/qgcoder.png DESTINATION share/icons/hicolor/256x256/apps)
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
