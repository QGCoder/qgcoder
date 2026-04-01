#include "view.h"

#include <limits>

#ifdef WIN32
#include <windows.h>
#endif

#include <GL/glut.h>

#include <QKeyEvent>
#include <QDebug>
#include <QWidget>
#include <QSurfaceFormat>

#include "canonMotion.hpp"

void View::keyPressEvent(QKeyEvent *e) {
    int keyInt = e->key();
    Qt::Key key = static_cast<Qt::Key>(keyInt);
    
    if (key == Qt::Key_unknown) {
        qDebug() << "Unknown key from a macro probably";
        return;
    }
    
    // the user have clicked just and only the special keys Ctrl, Shift, Alt, Meta.
    if (key == Qt::Key_Control ||
       key == Qt::Key_Shift ||
       key == Qt::Key_Alt ||
       key == Qt::Key_Meta)
    {
        // qDebug() << "Single click of special key: Ctrl, Shift, Alt or Meta";
        // qDebug() << "New KeySequence:" << QKeySequence(keyInt).toString(QKeySequence::NativeText);
        // return;
    }

    // check for a combination of user clicks
    Qt::KeyboardModifiers modifiers = e->modifiers();
    QString keyText = e->text();
    // if the keyText is empty than it's a special key like F1, F5, ...
    //  qDebug() << "Pressed Key:" << keyText;
    
    QList<Qt::Key> modifiersList;
    if (modifiers & Qt::ShiftModifier)
        keyInt += Qt::SHIFT;
    if (modifiers & Qt::ControlModifier)
        keyInt += Qt::CTRL;
    if (modifiers & Qt::AltModifier)
        keyInt += Qt::ALT;
    if (modifiers & Qt::MetaModifier)
        keyInt += Qt::META;
    
    QString seq = QKeySequence(keyInt).toString(QKeySequence::NativeText);
    // qDebug() << "KeySequence:" << seq;
    
    return; // skip built in command if overridden by shortcut

/*
  switch (e->key()) {
  case Qt::Key_S :
    break;
  case Qt::Key_P :
    break;
  case Qt::Key_C :
    resetCamView();
    break;
  default:
    QGLViewer::keyPressEvent(e);
}
*/
    QGLViewer::keyPressEvent(e);
}

View::View(QWidget *parent) : QGLViewer(parent)  {
  setAttribute(Qt::WA_DeleteOnClose);

  QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(0);
  format.setAlphaBufferSize(0);
  format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  setFormat(format);

  lines.reserve(20000);
}

View::~View() {
  // qDebug() << "View::~View()";
}

void View::close() {
  // qDebug() << "View::close()";
    // savePrefs();
  QGLViewer::close();
}

void View::clear() {
    //QMutexLocker locker(&mutex);

    // reset bounding box
    aabb[0] = std::numeric_limits<double>::max();
    aabb[1] = std::numeric_limits<double>::max();
    aabb[2] = std::numeric_limits<double>::max();
    
    aabb[3] = std::numeric_limits<double>::min();
    aabb[4] = std::numeric_limits<double>::min();
    aabb[5] = std::numeric_limits<double>::min();

    //setSceneRadius(1);
    
    lines.clear();
    dirty = false;
    updateGLViewer();
}

void View::resetCamView() {
    camera()->setPosition(initialCameraPosition);
    camera()->setOrientation(initialCameraOrientation);
    updateGLViewer();
}

void View::init() {
    setShortcut(DISPLAY_FPS, Qt::CTRL+Qt::Key_F);
    setShortcut(DRAW_GRID, 0);
    
    setManipulatedFrame(new ManipulatedFrame());

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_DEPTH_TEST);
    glDrawBuffer(GL_BACK);
    //glDisable(GL_CULL_FACE);
    //glLineStipple(2, 0xFFFF);
    glDisable(GL_LIGHTING);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //glLineWidth(3);
  
    setGridIsDrawn();
    setFPSIsDisplayed();
    setAxisIsDrawn();
    
    setMouseBinding(Qt::AltModifier, Qt::RightButton, SHOW_ENTIRE_SCENE, true, Qt::MiddleButton);

    clear();

    //startAnimation();
}

void View::initializeGL() {
    QGLViewer::initializeGL();
    //camera()->setZNearCoefficient(0.000001);
    //camera()->setZClippingCoefficient(1000.0);
}

void View::appendCanonLine(canonLine *l) {
    lines.push_back(l);
    dirty = true;

    if (l->isMotion()) {
        Point pos1 = l->point(0);
        Point pos2 = l->point(1);

        double oaabbmin[3], oaabbmax[3];

        oaabbmin[0] = qMin(pos1.x, pos2.x);
        oaabbmin[1] = qMin(pos1.y, pos2.y);
        oaabbmin[2] = qMin(pos1.z, pos2.z);

        oaabbmax[0] = qMax(pos1.x, pos2.x);
        oaabbmax[1] = qMax(pos1.y, pos2.y);
        oaabbmax[2] = qMax(pos1.z, pos2.z);

        for (int i = 0; i < 3; ++i) {
            aabb[  i] = qMin(aabb[  i], oaabbmin[i]);
            aabb[3+i] = qMax(aabb[3+i], oaabbmax[i]);
        }
    }
}

void View::drawObjects(bool simplified) {
    if (lines.empty())
        return;

    double step_size = simplified ? 0.75 : 0.1;
    double inv_ds = 1.0 / step_size;

    int skip_dyn = 1;
    double line_width = 3.0;

    if (lines.size() > 100000) {
        skip_dyn = 4;
        line_width = 1.0;
    } else if (lines.size() > 10000) {
        skip_dyn = 2;
        line_width = 2.0;
    }

    int skip = simplified ? skip_dyn * 2 : skip_dyn;

    std::vector<float> traverseVertices;
    std::vector<float> feedVertices;
    traverseVertices.reserve(lines.size() * 6);
    feedVertices.reserve(lines.size() * 6);

    for (size_t i = 0; i < lines.size(); i += skip) {
        g2m::canonLine *l = lines[i];
        if (!l->isMotion())
            continue;

        double move_length = l->length();
        int n_samples = std::max(static_cast<int>(move_length * inv_ds), 2);
        double interval_size = move_length / (n_samples - 1);

        for (int j = 0; j < n_samples - 1; ++j) {
            double t1 = j * interval_size;
            double t2 = (j + 1) * interval_size;
            Point pos1 = l->point(t1);
            Point pos2 = l->point(t2);

            float vertices[] = {
                static_cast<float>(pos1.x), static_cast<float>(pos1.y), static_cast<float>(pos1.z),
                static_cast<float>(pos2.x), static_cast<float>(pos2.y), static_cast<float>(pos2.z)
            };

            if (l->getMotionType() == TRAVERSE) {
                traverseVertices.insert(traverseVertices.end(), vertices, vertices + 6);
            } else {
                feedVertices.insert(feedVertices.end(), vertices, vertices + 6);
            }
        }
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, traverseVertices.data());

    glColor3f(0.0, 128.0f/255.0f, 0.0f);
    glLineWidth(2);
    glDrawArrays(GL_LINES, 0, traverseVertices.size() / 3);

    glVertexPointer(3, GL_FLOAT, 0, feedVertices.data());
    glColor3f(255.0f/255.0f, 215.0f/255.0f, 94.0f/255.0f);
    glLineWidth(static_cast<float>(line_width));
    glDrawArrays(GL_LINES, 0, feedVertices.size() / 3);

    glDisableClientState(GL_VERTEX_ARRAY);

    glLineWidth(2);
    glColor3f(1.0f, 0.0f, 0.0f);

    float aabbVerts[] = {
        static_cast<float>(aabb[0]), static_cast<float>(aabb[1]), static_cast<float>(aabb[2]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[1]), static_cast<float>(aabb[2]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[1]), static_cast<float>(aabb[2]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[1]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[1]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[0]), static_cast<float>(aabb[1]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[0]), static_cast<float>(aabb[1]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[0]), static_cast<float>(aabb[1]), static_cast<float>(aabb[2]),

        static_cast<float>(aabb[0]), static_cast<float>(aabb[4]), static_cast<float>(aabb[2]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[4]), static_cast<float>(aabb[2]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[4]), static_cast<float>(aabb[2]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[4]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[4]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[0]), static_cast<float>(aabb[4]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[0]), static_cast<float>(aabb[4]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[0]), static_cast<float>(aabb[4]), static_cast<float>(aabb[2]),

        static_cast<float>(aabb[0]), static_cast<float>(aabb[1]), static_cast<float>(aabb[2]),
        static_cast<float>(aabb[0]), static_cast<float>(aabb[4]), static_cast<float>(aabb[2]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[1]), static_cast<float>(aabb[2]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[4]), static_cast<float>(aabb[2]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[1]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[3]), static_cast<float>(aabb[4]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[0]), static_cast<float>(aabb[1]), static_cast<float>(aabb[5]),
        static_cast<float>(aabb[0]), static_cast<float>(aabb[4]), static_cast<float>(aabb[5])
    };

    glVertexPointer(3, GL_FLOAT, 0, aabbVerts);
    glDrawArrays(GL_LINES, 0, 24);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void View::draw() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    drawObjects(false);
}

void View::fastDraw() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    drawObjects(true);
}

void View::postDraw() {
  QGLViewer::postDraw();
}

void View::update() {
    // qDebug() << "View::update";

    if (_autoZoom)
        showEntireScene();

    if (dirty) {
        dirty = false;
        updateGLViewer();
    }
}
