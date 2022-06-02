TARGET   = qgcoder

TEMPLATE = app

CONFIG      += link_pkgconfig

QMAKE_CXXFLAGS_RELEASE += -O2
QMAKE_CXXFLAGS_DEBUG   += -O0

MOC_DIR = .moc
#OBJECTS_DIR = .obj
UI_DIR = .ui
RCC_DIR = .rcc

CONFIG += qgcodeeditor
CONFIG += qglviewer

INCLUDEPATH += g2m

HEADERS     = \
    mainwin.h \
    view.h \
    g2m/canonLine.hpp    g2m/canonMotionless.hpp  g2m/gplayer.hpp        g2m/linearMotion.hpp   g2m/nanotimer.hpp \
    g2m/canonMotion.hpp  g2m/g2m.hpp              g2m/helicalMotion.hpp  g2m/machineStatus.hpp  g2m/point.hpp \
    lex_analyzer.hpp	settings_dlg.h
    

SOURCES     = \
    main.cpp \
    mainwin.cpp \
    view.cpp \
    g2m/canonLine.cpp    g2m/canonMotionless.cpp  g2m/helicalMotion.cpp  g2m/machineStatus.cpp \
    g2m/canonMotion.cpp  g2m/g2m.cpp              g2m/linearMotion.cpp   g2m/nanotimer.cpp \
    lex_analyzer.cpp     settings_dlg.cpp

target.path = /usr/bin

INSTALLS += target

FORMS += \
    mainwin.ui settings.ui

link_pkgconfig {
#  message("Using pkg-config "$$system(pkg-config --version)".")

  LSB_RELEASE_ID  = $$system(. /etc/os-release; echo "$NAME")
  LSB_RELEASE_REL = $$system(. /etc/os-release; echo "$VERSION_ID")

  message(This is $$LSB_RELEASE_ID $$LSB_RELEASE_REL)

  contains(LSB_RELEASE_ID, Ubuntu): {
    contains(LSB_RELEASE_REL, 21.04) : {
      LIBS += -lQGLViewer-qt5 -lGLEW -lGLU -lGL -lglut
    }
    contains(LSB_RELEASE_REL, 22.04) : {
      LIBS += -lQGLViewer-qt5 -lGLEW -lGLU -lGL -lglut
    }
  } else {
    LIBS += -lQGLViewer -lGLEW -lGLU -lGL -lglut
  }
}

CONFIG  *= debug_and_release
CONFIG  *= qt opengl
CONFIG  += warn_on
CONFIG  += thread

QT      *= opengl xml gui core

OTHER_FILES += README.md

DIRS_DC = object_script.* .ui .moc .rcc .obj *.pro.user $$TARGET

unix:QMAKE_DISTCLEAN  += -r $$DIRS_DC
win32:QMAKE_DISTCLEAN += /s /f /q $$DIRS_DC && rd /s /q $$DIRS_DC

DISTFILES += \
    icons/qgcoder.png

RESOURCES += \
    res.qrc

