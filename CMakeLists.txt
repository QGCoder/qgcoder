cmake_minimum_required(VERSION 3.21)
project(qgcoder LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR})

find_package(Qt5 REQUIRED COMPONENTS Core Gui OpenGL Xml)
find_package(OpenGL REQUIRED)

find_library(QGLVIEWER_LIBRARY NAMES QGLViewer-qt5 QGLViewer REQUIRED)
find_library(GLEW_LIBRARY NAMES GLEW REQUIRED)
find_library(GLU_LIBRARY NAMES GLU REQUIRED)
find_library(GL_LIBRARY NAMES GL REQUIRED)
find_library(GLUT_LIBRARY NAMES glut freeglut REQUIRED)

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
target_include_directories(rs274ngc PUBLIC ${CMAKE_SOURCE_DIR}/rs274ngc)
# only the vendored files get warnings turned off; rs274ngc_interp.cpp is ours
set_source_files_properties(
    rs274ngc/rs274ngc_pre.cc
    rs274ngc/canon_pre.cc
    PROPERTIES COMPILE_OPTIONS "-w"
)

find_library(QGCODEEDITOR_LIBRARY NAMES qgcodeeditor_qt5 qgcodeeditor)
find_path(QGCODEEDITOR_INCLUDE_DIR NAMES QGCodeEditor/QGCodeEditor.h
  PATHS /usr/include /usr/local/include
)

set(SOURCES
    main.cpp
    mainwin.cpp
    view.cpp
    lex_analyzer.cpp
    settings_dlg.cpp
    g2m/canonLine.cpp
    g2m/canonMotion.cpp
    g2m/canonMotionless.cpp
    g2m/g2m.cpp
    g2m/g2mworker.cpp
    g2m/helicalMotion.cpp
    g2m/linearMotion.cpp
    g2m/machineStatus.cpp
    g2m/nanotimer.cpp
)

set(HEADERS
    mainwin.h
    view.h
    lex_analyzer.hpp
    settings_dlg.h
    g2m/canonLine.hpp
    g2m/canonMotion.hpp
    g2m/canonMotionless.hpp
    g2m/g2m.hpp
    g2m/g2mworker.h
    g2m/gplayer.hpp
    g2m/helicalMotion.hpp
    g2m/linearMotion.hpp
    g2m/machineStatus.hpp
    g2m/nanotimer.hpp
    g2m/point.hpp
)

set(FORMS
    mainwin.ui
    settings.ui
)

set(RESOURCES
    res.qrc
)

add_executable(qgcoder
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
    ${RESOURCES}
)

target_include_directories(qgcoder PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/g2m
    /usr/include/QGCodeEditor
)

target_compile_options(qgcoder PRIVATE
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Debug>:-O0>
    -Wno-deprecated-copy
)

target_link_libraries(qgcoder PRIVATE
    rs274ngc
    Qt5::Core
    Qt5::Gui
    Qt5::OpenGL
    Qt5::Xml
    OpenGL::GL
    OpenGL::GLU
    ${QGLVIEWER_LIBRARY}
    ${GLEW_LIBRARY}
    ${GLUT_LIBRARY}
)

if(QGCODEEDITOR_LIBRARY)
    target_link_libraries(qgcoder PRIVATE ${QGCODEEDITOR_LIBRARY})
    if(QGCODEEDITOR_INCLUDE_DIR)
        target_include_directories(qgcoder PRIVATE ${QGCODEEDITOR_INCLUDE_DIR})
    endif()
endif()

install(TARGETS qgcoder RUNTIME DESTINATION bin)
install(FILES qgcoder.desktop DESTINATION share/applications)
install(FILES icons/qgcoder.png DESTINATION share/icons/hicolor/256x256/apps)
