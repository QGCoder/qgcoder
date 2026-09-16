#ifndef VIEW_H
#define VIEW_H

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QMutex>
#ifdef Q_OS_WASM
#  include <QWidget>
#else
#  include <QOpenGLBuffer>
#  include <QOpenGLExtraFunctions>
#  include <QOpenGLShaderProgram>
#  include <QOpenGLVertexArrayObject>
#  include <QOpenGLWidget>
#endif
#include <QPoint>
#include <QQuaternion>
#include <QVector3D>

#include <vector>

#include "canonLine.hpp"

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

/// \brief 3D tool-path view.
///
/// A self-contained Qt 6 viewer: QOpenGLWidget plus a shader pipeline, so
/// there is no dependency on libQGLViewer (which has no Qt 6 build) and none
/// on the fixed-function pipeline it relied on. The tool path is tessellated
/// once into vertex buffers when it changes rather than on every frame, and
/// drawn with a single trivial shader.
///
/// QOpenGLExtraFunctions rather than QOpenGLFunctions_3_3_Core: everything
/// here is in the subset shared by an OpenGL 3.3 core profile and OpenGL
/// ES 3.0.
///
/// Emscripten is the exception and draws the same geometry with QPainter
/// instead. QOpenGLWidget does not work in Qt for WebAssembly: the widget
/// renders into its own WebGL context, and the compositor then has to wrap
/// that context's texture for the one it composes the window in - which WebGL,
/// having no context sharing, cannot do. Both contexts are lost the moment it
/// is tried. There is nothing to accelerate here but coloured line segments,
/// so projecting them on the CPU costs little.
#ifdef Q_OS_WASM
class View : public QWidget
#else
class View : public QOpenGLWidget, protected QOpenGLExtraFunctions
#endif
{
    Q_OBJECT
    Q_PROPERTY(bool autoZoom READ autoZoom WRITE setAutoZoom RESET unsetAutoZoom NOTIFY autoZoomChanged)

public:
    explicit View(QWidget *parent = nullptr);
    ~View() override;

    [[nodiscard]] bool autoZoom() const { return m_autoZoom; }
    void setAutoZoom(bool autoZoom);
    void unsetAutoZoom() { setAutoZoom(true); }

    [[nodiscard]] bool axisIsDrawn() const { return m_drawAxis; }
    [[nodiscard]] bool gridIsDrawn() const { return m_drawGrid; }

public slots:
    /// replace the whole tool path
    void setCanonLines(const QVector<g2m::canonLine *> &lines);
    /// append a single canon line to the tool path
    void appendCanonLine(g2m::canonLine *l);
    /// drop the tool path and reset the bounding box
    void clear();

    /// re-fit (when auto-zoom is on) and repaint
    void refresh();

    /// frame the whole tool path
    void showEntireScene();
    /// back to the default viewpoint
    void resetCamView();

    void setAxisIsDrawn(bool draw = true);
    void setGridIsDrawn(bool draw = true);

signals:
    void autoZoomChanged(bool autoZoom);
    /// render rate, refreshed roughly twice a second while drawing
    void fpsChanged(double fps);

protected:
#ifdef Q_OS_WASM
    void paintEvent(QPaintEvent *e) override;
#else
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
#endif

    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    /// release the GPU resources; safe to call more than once
    void cleanupGl();
    /// tessellate the canon lines into m_traverseVerts / m_feedVerts
    void rebuildGeometry();
    /// bring the vertex data up to date, and on a GPU build push it over
    void uploadGeometry();
    void rebuildDecorations(std::vector<float> &out);
    void resetBounds();
    /// grow the bounding box to hold one motion
    void accumulateBounds(g2m::canonLine *l);
    /// derive scene centre and radius from the bounding box
    void applyBounds();

    [[nodiscard]] QMatrix4x4 viewMatrix() const;
    [[nodiscard]] QMatrix4x4 projectionMatrix() const;
#ifdef Q_OS_WASM
    /// project a run of vertex pairs and stroke them
    void drawLines(QPainter &p, const QMatrix4x4 &mvp, const std::vector<float> &verts,
                   const QColor &color, int first, int count);
#else
    void drawLines(const QMatrix4x4 &mvp, const QColor &color, int first, int count);
#endif

    void orbit(QPointF delta);
    void pan(QPointF delta);
    void zoom(float steps);

    // --- tool path -------------------------------------------------------
    mutable QMutex m_mutex;
    std::vector<g2m::canonLine *> m_lines;
    std::vector<float> m_traverseVerts;
    std::vector<float> m_feedVerts;
    bool m_geometryDirty = true;
    bool m_decorDirty = true;

    QVector3D m_boundsMin;
    QVector3D m_boundsMax;
    bool m_boundsValid = false;

    /// the grid, axes and bounding box, in the same xyz-triple form
    std::vector<float> m_decorVerts;

    // --- GPU resources ---------------------------------------------------
#ifndef Q_OS_WASM
    QOpenGLShaderProgram m_program;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_pathVbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_decorVbo{QOpenGLBuffer::VertexBuffer};
    int m_mvpLoc = -1;
    int m_colorLoc = -1;
#endif
    int m_traverseCount = 0; ///< vertices, offset 0 of m_pathVbo
    int m_feedCount = 0;     ///< vertices, right after the traverse ones
    int m_gridFirst = 0;
    int m_gridCount = 0;
    int m_axisFirst = 0;
    int m_boxFirst = 0;
    bool m_glReady = false;
    /// true between initializeGL() and cleanupGl(): there are buffers to free
    bool m_glOwned = false;

    // --- camera ----------------------------------------------------------
    QQuaternion m_orientation;
    QVector3D m_target;
    QVector3D m_sceneCenter;
    float m_sceneRadius = 1.0f;
    float m_distance = 4.0f;
    float m_fovY = 45.0f;

    QPoint m_lastPos;
    Qt::MouseButton m_dragButton = Qt::NoButton;

    // --- display flags ---------------------------------------------------
    bool m_autoZoom = true;
    bool m_drawAxis = true;
    bool m_drawGrid = true;

    QElapsedTimer m_fpsTimer;
    int m_fpsFrames = 0;
    double m_fps = 0.0;
};

#endif // VIEW_H
