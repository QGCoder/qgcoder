/// \file
/// The 3D tool-path view.

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
    /// whether the view reframes itself whenever the tool path changes
    Q_PROPERTY(bool autoZoom READ autoZoom WRITE setAutoZoom RESET unsetAutoZoom NOTIFY autoZoomChanged)

public:
    /// \param parent widget parent
    explicit View(QWidget *parent = nullptr);
    /// Releases the GPU resources, if the context has not already gone.
    ~View() override;

    /// \returns true while the view reframes itself on every change
    [[nodiscard]] bool autoZoom() const { return m_autoZoom; }
    void setAutoZoom(bool autoZoom);
    /// Restores auto-zoom to its default, which is on.
    void unsetAutoZoom() { setAutoZoom(true); }

    /// \returns true while the origin axes are drawn
    [[nodiscard]] bool axisIsDrawn() const { return m_drawAxis; }
    /// \returns true while the XY grid is drawn
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
    /// auto-zoom was switched \param autoZoom its new state
    void autoZoomChanged(bool autoZoom);
    /// render rate, refreshed roughly twice a second while drawing
    void fpsChanged(double fps);

protected:
#ifdef Q_OS_WASM
    /// \copydoc View::paintEvent
    void paintEvent(QPaintEvent *e) override;
#else
    /// set up the shader, VAO and buffers
    void initializeGL() override;
    /// resize the GL viewport
    void resizeGL(int w, int h) override;
    /// draw the decorations and the tool path
    void paintGL() override;
#endif

    /// start a drag
    void mousePressEvent(QMouseEvent *e) override;
    /// orbit, pan or zoom the drag in progress
    void mouseMoveEvent(QMouseEvent *e) override;
    /// end the drag
    void mouseReleaseEvent(QMouseEvent *e) override;
    /// frame the whole path
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    /// zoom
    void wheelEvent(QWheelEvent *e) override;
    /// the view's keyboard shortcuts
    void keyPressEvent(QKeyEvent *e) override;

private:
    /// release the GPU resources; safe to call more than once
    void cleanupGl();
    /// tessellate the canon lines into m_traverseVerts / m_feedVerts
    void rebuildGeometry();
    /// bring the vertex data up to date, and on a GPU build push it over
    void uploadGeometry();
    /// build the grid and axes
    void rebuildDecorations(std::vector<float> &out);
    /// forget the bounding box
    void resetBounds();
    /// grow the bounding box to hold one motion
    void accumulateBounds(g2m::canonLine *l);
    /// derive scene centre and radius from the bounding box
    void applyBounds();

    /// \returns the world-to-eye transform
    [[nodiscard]] QMatrix4x4 viewMatrix() const;
    /// \returns the perspective projection
    [[nodiscard]] QMatrix4x4 projectionMatrix() const;
#ifdef Q_OS_WASM
    /// project a run of vertex pairs and stroke them
    void drawLines(QPainter &p, const QMatrix4x4 &mvp, const std::vector<float> &verts,
                   const QColor &color, int first, int count);
#else
    /// draw one run of line vertices in a single colour
    void drawLines(const QMatrix4x4 &mvp, const QColor &color, int first, int count);
#endif

    /// turn the camera about the target
    void orbit(QPointF delta);
    /// slide the target across the view plane
    void pan(QPointF delta);
    /// move the camera along its line of sight
    void zoom(float steps);

    // --- tool path -------------------------------------------------------
    mutable QMutex m_mutex;          ///< guards the tool path and its bounds
    std::vector<g2m::canonLine *> m_lines;  ///< the tool path; not owned
    std::vector<float> m_traverseVerts;     ///< rapid vertices, xyz triples
    std::vector<float> m_feedVerts;         ///< cutting vertices, xyz triples
    bool m_geometryDirty = true;     ///< the tool path needs tessellating again
    bool m_decorDirty = true;        ///< the grid and axes need rebuilding

    QVector3D m_boundsMin;           ///< low corner of the tool path
    QVector3D m_boundsMax;           ///< high corner of the tool path
    bool m_boundsValid = false;      ///< false until a motion has been seen

    /// the grid, axes and bounding box, in the same xyz-triple form
    std::vector<float> m_decorVerts;

    // --- GPU resources ---------------------------------------------------
#ifndef Q_OS_WASM
    QOpenGLShaderProgram m_program;  ///< the one shader everything is drawn with
    QOpenGLVertexArrayObject m_vao;  ///< vertex array state
    QOpenGLBuffer m_pathVbo{QOpenGLBuffer::VertexBuffer};   ///< tool path vertices
    QOpenGLBuffer m_decorVbo{QOpenGLBuffer::VertexBuffer};  ///< grid and axes
    int m_mvpLoc = -1;               ///< uniform location of the transform
    int m_colorLoc = -1;             ///< uniform location of the colour
#endif
    int m_traverseCount = 0; ///< vertices, offset 0 of m_pathVbo
    int m_feedCount = 0;     ///< vertices, right after the traverse ones
    int m_gridFirst = 0;             ///< first grid vertex in m_decorVbo
    int m_gridCount = 0;             ///< how many grid vertices
    int m_axisFirst = 0;             ///< first axis vertex, six of them
    int m_boxFirst = 0;              ///< first bounding-box vertex
    bool m_glReady = false;          ///< the context and shader are usable
    /// true between initializeGL() and cleanupGl(): there are buffers to free
    bool m_glOwned = false;

    // --- camera ----------------------------------------------------------
    QQuaternion m_orientation;       ///< which way the camera faces
    QVector3D m_target;              ///< the point the camera looks at
    QVector3D m_sceneCenter;         ///< middle of the tool path
    float m_sceneRadius = 1.0f;      ///< radius of the sphere holding it
    float m_distance = 4.0f;         ///< camera distance from the target
    float m_fovY = 45.0f;            ///< vertical field of view, degrees

    QPoint m_lastPos;                ///< cursor at the last mouse event
    Qt::MouseButton m_dragButton = Qt::NoButton;  ///< button driving the drag

    // --- display flags ---------------------------------------------------
    bool m_autoZoom = true;          ///< reframe whenever the path changes
    bool m_drawAxis = true;          ///< draw the origin axes
    bool m_drawGrid = true;          ///< draw the XY grid

    QElapsedTimer m_fpsTimer;        ///< since the last fpsChanged()
    int m_fpsFrames = 0;             ///< frames drawn since then
    double m_fps = 0.0;              ///< the rate last reported
};

#endif // VIEW_H
