#ifndef VIEW_H
#define VIEW_H

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QMutex>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPoint>
#include <QQuaternion>
#include <QVector3D>

#include <vector>

#include "canonLine.hpp"

/// \brief 3D tool-path view.
///
/// A self-contained Qt 6 viewer: QOpenGLWidget plus the OpenGL 3.3 core
/// profile, so there is no dependency on libQGLViewer (which has no Qt 6
/// build) and none on the fixed-function pipeline it relied on. The tool path
/// is tessellated once into vertex buffers when it changes rather than on
/// every frame, and drawn with a single trivial shader.
class View : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
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
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

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
    /// push the CPU-side vertex data into the GPU buffers
    void uploadGeometry();
    void rebuildDecorations(std::vector<float> &out);
    void resetBounds();
    /// grow the bounding box to hold one motion
    void accumulateBounds(g2m::canonLine *l);
    /// derive scene centre and radius from the bounding box
    void applyBounds();

    [[nodiscard]] QMatrix4x4 viewMatrix() const;
    [[nodiscard]] QMatrix4x4 projectionMatrix() const;
    void drawLines(const QMatrix4x4 &mvp, const QColor &color, int first, int count);

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

    // --- GPU resources ---------------------------------------------------
    QOpenGLShaderProgram m_program;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_pathVbo{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer m_decorVbo{QOpenGLBuffer::VertexBuffer};
    int m_mvpLoc = -1;
    int m_colorLoc = -1;
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
