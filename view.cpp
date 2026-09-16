#include "view.h"

#include <QColor>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMutexLocker>
#include <QWheelEvent>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

constexpr auto kVertexShader = R"(#version 330 core
layout(location = 0) in vec3 a_position;
uniform mat4 u_mvp;
void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

constexpr auto kFragmentShader = R"(#version 330 core
uniform vec4 u_color;
out vec4 fragColor;
void main() {
    fragColor = u_color;
}
)";

const QColor kBackground(0, 0, 60);
const QColor kTraverseColor(0, 128, 0);
const QColor kFeedColor(255, 215, 94);
const QColor kBoxColor(255, 255, 255);
const QColor kGridColor(48, 48, 120);
const QColor kAxisXColor(220, 60, 60);
const QColor kAxisYColor(60, 200, 60);
const QColor kAxisZColor(80, 120, 255);

void appendVertex(std::vector<float> &out, const g2m::Point &p)
{
    out.push_back(static_cast<float>(p.x));
    out.push_back(static_cast<float>(p.y));
    out.push_back(static_cast<float>(p.z));
}

void appendVertex(std::vector<float> &out, float x, float y, float z)
{
    out.push_back(x);
    out.push_back(y);
    out.push_back(z);
}

/// a "nice" grid spacing (1, 2 or 5 times a power of ten) near span / 10
float niceStep(float span)
{
    if (!(span > 0.0f))
        return 1.0f;
    const float raw = span / 10.0f;
    const float mag = std::pow(10.0f, std::floor(std::log10(raw)));
    const float norm = raw / mag;
    if (norm < 1.5f)
        return mag;
    if (norm < 3.5f)
        return 2.0f * mag;
    if (norm < 7.5f)
        return 5.0f * mag;
    return 10.0f * mag;
}

} // namespace

View::View(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    m_lines.reserve(20000);
    resetBounds();
    resetCamView();
    m_fpsTimer.start();
}

View::~View()
{
    cleanupGl();
}

/// Both ~View() and QOpenGLContext::aboutToBeDestroyed lead here, and either
/// can come first, so the buffers must only ever be destroyed once.
void View::cleanupGl()
{
    if (!m_glOwned)
        return;
    m_glOwned = false;
    m_glReady = false;

    makeCurrent();
    m_pathVbo.destroy();
    m_decorVbo.destroy();
    m_vao.destroy();
    m_program.removeAllShaders();
    doneCurrent();
}

// ---------------------------------------------------------------------------
// tool path
// ---------------------------------------------------------------------------

void View::clear()
{
    {
        const QMutexLocker locker(&m_mutex);
        m_lines.clear();
        m_traverseVerts.clear();
        m_feedVerts.clear();
        m_geometryDirty = true;
        resetBounds();
    }
    applyBounds();
    update();
}

void View::setCanonLines(const QVector<g2m::canonLine *> &lines)
{
    {
        const QMutexLocker locker(&m_mutex);
        m_lines.assign(lines.begin(), lines.end());
        resetBounds();
        for (g2m::canonLine *l : m_lines)
            accumulateBounds(l);
        m_geometryDirty = true;
    }
    applyBounds();
    showEntireScene();
    update();
}

void View::appendCanonLine(g2m::canonLine *l)
{
    if (!l)
        return;
    {
        const QMutexLocker locker(&m_mutex);
        m_lines.push_back(l);
        accumulateBounds(l);
        m_geometryDirty = true;
    }
    applyBounds();
    if (m_autoZoom)
        showEntireScene();
    update();
}

void View::refresh()
{
    if (m_autoZoom)
        showEntireScene();
    update();
}

void View::resetBounds()
{
    m_boundsMin = QVector3D(0.0f, 0.0f, 0.0f);
    m_boundsMax = QVector3D(0.0f, 0.0f, 0.0f);
    m_boundsValid = false;
}

void View::accumulateBounds(g2m::canonLine *l)
{
    if (!l || !l->isMotion())
        return;

    const g2m::Point a = l->point(0.0);
    const g2m::Point b = l->point(l->length());

    for (const g2m::Point &p : {a, b}) {
        const QVector3D v(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z));
        if (!m_boundsValid) {
            m_boundsMin = m_boundsMax = v;
            m_boundsValid = true;
        } else {
            m_boundsMin = QVector3D(std::min(m_boundsMin.x(), v.x()),
                                    std::min(m_boundsMin.y(), v.y()),
                                    std::min(m_boundsMin.z(), v.z()));
            m_boundsMax = QVector3D(std::max(m_boundsMax.x(), v.x()),
                                    std::max(m_boundsMax.y(), v.y()),
                                    std::max(m_boundsMax.z(), v.z()));
        }
    }
}

void View::applyBounds()
{
    QVector3D lo, hi;
    bool valid = false;
    {
        const QMutexLocker locker(&m_mutex);
        lo = m_boundsMin;
        hi = m_boundsMax;
        valid = m_boundsValid;
    }

    m_sceneCenter = valid ? (lo + hi) * 0.5f : QVector3D();
    m_sceneRadius = valid ? std::max((hi - lo).length() * 0.5f, 1e-3f) : 1.0f;
    m_decorDirty = true;
}

void View::setAutoZoom(bool autoZoom)
{
    if (m_autoZoom == autoZoom)
        return;
    m_autoZoom = autoZoom;
    emit autoZoomChanged(m_autoZoom);
    if (m_autoZoom)
        showEntireScene();
    update();
}

void View::setAxisIsDrawn(bool draw)
{
    m_drawAxis = draw;
    update();
}

void View::setGridIsDrawn(bool draw)
{
    m_drawGrid = draw;
    update();
}

// ---------------------------------------------------------------------------
// geometry
// ---------------------------------------------------------------------------

void View::rebuildGeometry()
{
    const QMutexLocker locker(&m_mutex);

    m_traverseVerts.clear();
    m_feedVerts.clear();
    if (m_lines.empty())
        return;

    // Straight moves are exact with two samples; only arcs and helices need to
    // be flattened, and how finely depends on how big the part is on screen.
    const float span = m_boundsValid ? (m_boundsMax - m_boundsMin).length() : 1.0f;
    const double chord = std::max(static_cast<double>(span) / 2000.0, 1e-3);

    m_traverseVerts.reserve(m_lines.size() * 6);
    m_feedVerts.reserve(m_lines.size() * 6);

    for (g2m::canonLine *l : m_lines) {
        if (!l->isMotion())
            continue;

        const double length = l->length();
        if (!(length > 0.0))
            continue;

        int samples = 2;
        if (l->getMotionType() == g2m::HELICAL)
            samples = std::clamp(static_cast<int>(std::ceil(length / chord)) + 1, 2, 4096);

        std::vector<float> &out =
            (l->getMotionType() == g2m::TRAVERSE) ? m_traverseVerts : m_feedVerts;

        const double interval = length / (samples - 1);
        g2m::Point prev = l->point(0.0);
        for (int i = 1; i < samples; ++i) {
            const g2m::Point cur = l->point(i * interval);
            appendVertex(out, prev);
            appendVertex(out, cur);
            prev = cur;
        }
    }
}

void View::rebuildDecorations(std::vector<float> &out)
{
    out.clear();

    // grid in the XY plane, the plane most G-code is written in
    const float step = niceStep(m_sceneRadius * 2.0f);
    const int halfLines = 10;
    const float extent = step * halfLines;
    m_gridFirst = 0;
    for (int i = -halfLines; i <= halfLines; ++i) {
        const float t = step * i;
        appendVertex(out, t, -extent, 0.0f);
        appendVertex(out, t, extent, 0.0f);
        appendVertex(out, -extent, t, 0.0f);
        appendVertex(out, extent, t, 0.0f);
    }
    m_gridCount = static_cast<int>(out.size() / 3);

    // axes
    m_axisFirst = m_gridCount;
    const float axisLen = extent;
    appendVertex(out, 0.0f, 0.0f, 0.0f);
    appendVertex(out, axisLen, 0.0f, 0.0f);
    appendVertex(out, 0.0f, 0.0f, 0.0f);
    appendVertex(out, 0.0f, axisLen, 0.0f);
    appendVertex(out, 0.0f, 0.0f, 0.0f);
    appendVertex(out, 0.0f, 0.0f, axisLen);

    // bounding box of the tool path
    m_boxFirst = static_cast<int>(out.size() / 3);
    const QVector3D lo = m_boundsMin;
    const QVector3D hi = m_boundsMax;
    const QVector3D corner[8] = {
        {lo.x(), lo.y(), lo.z()}, {hi.x(), lo.y(), lo.z()},
        {hi.x(), hi.y(), lo.z()}, {lo.x(), hi.y(), lo.z()},
        {lo.x(), lo.y(), hi.z()}, {hi.x(), lo.y(), hi.z()},
        {hi.x(), hi.y(), hi.z()}, {lo.x(), hi.y(), hi.z()},
    };
    constexpr int edges[24] = {0, 1, 1, 2, 2, 3, 3, 0,
                               4, 5, 5, 6, 6, 7, 7, 4,
                               0, 4, 1, 5, 2, 6, 3, 7};
    for (const int idx : edges)
        appendVertex(out, corner[idx].x(), corner[idx].y(), corner[idx].z());
}

void View::uploadGeometry()
{
    if (m_geometryDirty) {
        rebuildGeometry();
        m_geometryDirty = false;

        const QMutexLocker locker(&m_mutex);
        m_traverseCount = static_cast<int>(m_traverseVerts.size() / 3);
        m_feedCount = static_cast<int>(m_feedVerts.size() / 3);

        m_pathVbo.bind();
        const auto traverseBytes = static_cast<int>(m_traverseVerts.size() * sizeof(float));
        const auto feedBytes = static_cast<int>(m_feedVerts.size() * sizeof(float));
        m_pathVbo.allocate(traverseBytes + feedBytes);
        if (traverseBytes > 0)
            m_pathVbo.write(0, m_traverseVerts.data(), traverseBytes);
        if (feedBytes > 0)
            m_pathVbo.write(traverseBytes, m_feedVerts.data(), feedBytes);
        m_pathVbo.release();
    }

    if (m_decorDirty) {
        std::vector<float> decor;
        rebuildDecorations(decor);
        m_decorDirty = false;

        m_decorVbo.bind();
        m_decorVbo.allocate(decor.data(), static_cast<int>(decor.size() * sizeof(float)));
        m_decorVbo.release();
    }
}

// ---------------------------------------------------------------------------
// camera
// ---------------------------------------------------------------------------

void View::resetCamView()
{
    // an isometric-ish viewpoint: Z up, looking from the front, right and above
    m_orientation = QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, 30.0f)
                    * QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 60.0f);
    showEntireScene();
}

void View::showEntireScene()
{
    m_target = m_sceneCenter;

    const float halfFovY = qDegreesToRadians(m_fovY) * 0.5f;
    const float aspect = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
    const float halfFovX = std::atan(std::tan(halfFovY) * aspect);

    const float distV = m_sceneRadius / std::sin(halfFovY);
    const float distH = m_sceneRadius / std::sin(std::max(halfFovX, 1e-3f));
    m_distance = std::max(distV, distH) * 1.05f;
    update();
}

QMatrix4x4 View::viewMatrix() const
{
    QMatrix4x4 m;
    m.translate(0.0f, 0.0f, -m_distance);
    m.rotate(m_orientation.conjugated());
    m.translate(-m_target);
    return m;
}

QMatrix4x4 View::projectionMatrix() const
{
    const float aspect = height() > 0 ? static_cast<float>(width()) / static_cast<float>(height()) : 1.0f;
    // keep the whole scene between the clipping planes wherever the camera is
    const float reach = m_distance + (m_target - m_sceneCenter).length() + m_sceneRadius;
    const float zFar = std::max(reach * 2.0f, 1e-2f);
    const float zNear = std::max(zFar * 1e-4f, m_distance - m_sceneRadius * 2.0f);

    QMatrix4x4 m;
    m.perspective(m_fovY, aspect, zNear, zFar);
    return m;
}

void View::orbit(QPointF delta)
{
    constexpr float kDegPerPixel = 0.4f;
    const QVector3D right = m_orientation.rotatedVector(QVector3D(1.0f, 0.0f, 0.0f));
    const QVector3D up = m_orientation.rotatedVector(QVector3D(0.0f, 1.0f, 0.0f));

    const QQuaternion dq =
        QQuaternion::fromAxisAndAngle(right, static_cast<float>(delta.y()) * kDegPerPixel)
        * QQuaternion::fromAxisAndAngle(up, static_cast<float>(delta.x()) * kDegPerPixel);

    m_orientation = (dq * m_orientation).normalized();
    update();
}

void View::pan(QPointF delta)
{
    if (height() <= 0)
        return;
    // one pixel of drag moves the scene by one pixel at the depth of the target
    const float halfFovY = qDegreesToRadians(m_fovY) * 0.5f;
    const float scale = 2.0f * m_distance * std::tan(halfFovY) / static_cast<float>(height());

    const QVector3D right = m_orientation.rotatedVector(QVector3D(1.0f, 0.0f, 0.0f));
    const QVector3D up = m_orientation.rotatedVector(QVector3D(0.0f, 1.0f, 0.0f));

    m_target -= right * (static_cast<float>(delta.x()) * scale);
    m_target += up * (static_cast<float>(delta.y()) * scale);
    update();
}

void View::zoom(float steps)
{
    m_distance *= std::pow(1.1f, -steps);
    m_distance = std::clamp(m_distance, m_sceneRadius * 1e-3f, m_sceneRadius * 1e4f);
    update();
}

// ---------------------------------------------------------------------------
// OpenGL
// ---------------------------------------------------------------------------

void View::initializeGL()
{
    if (!initializeOpenGLFunctions()) {
        qWarning("View: an OpenGL 3.3 core profile context is required");
        return;
    }

    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &View::cleanupGl,
            Qt::DirectConnection);

    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShader)
        || !m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentShader)
        || !m_program.link()) {
        qWarning("View: shader error: %s", qPrintable(m_program.log()));
        return;
    }

    m_mvpLoc = m_program.uniformLocation("u_mvp");
    m_colorLoc = m_program.uniformLocation("u_color");

    m_vao.create();
    m_pathVbo.create();
    m_pathVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    m_decorVbo.create();
    m_decorVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(kBackground.redF(), kBackground.greenF(), kBackground.blueF(), 1.0f);

    m_glReady = true;
    m_glOwned = true;
    m_geometryDirty = true;
    m_decorDirty = true;
}

void View::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void View::drawLines(const QMatrix4x4 &mvp, const QColor &color, int first, int count)
{
    if (count <= 0)
        return;
    m_program.setUniformValue(m_mvpLoc, mvp);
    m_program.setUniformValue(m_colorLoc, QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF()));
    glDrawArrays(GL_LINES, first, count);
}

void View::paintGL()
{
    if (m_glReady) {
        uploadGeometry();

        glEnable(GL_DEPTH_TEST);
        glClearColor(kBackground.redF(), kBackground.greenF(), kBackground.blueF(), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const QMatrix4x4 mvp = projectionMatrix() * viewMatrix();

        m_program.bind();
        {
            const QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

            // decorations: grid, axes, bounding box
            m_decorVbo.bind();
            m_program.enableAttributeArray(0);
            m_program.setAttributeBuffer(0, GL_FLOAT, 0, 3);

            glLineWidth(1.0f);
            if (m_drawGrid)
                drawLines(mvp, kGridColor, m_gridFirst, m_gridCount);
            if (m_drawAxis) {
                drawLines(mvp, kAxisXColor, m_axisFirst + 0, 2);
                drawLines(mvp, kAxisYColor, m_axisFirst + 2, 2);
                drawLines(mvp, kAxisZColor, m_axisFirst + 4, 2);
            }
            if (m_boundsValid)
                drawLines(mvp, kBoxColor, m_boxFirst, 24);
            m_decorVbo.release();

            // the tool path itself
            m_pathVbo.bind();
            m_program.enableAttributeArray(0);
            m_program.setAttributeBuffer(0, GL_FLOAT, 0, 3);

            glLineWidth(2.0f);
            drawLines(mvp, kTraverseColor, 0, m_traverseCount);
            drawLines(mvp, kFeedColor, m_traverseCount, m_feedCount);
            m_pathVbo.release();
        }
        m_program.release();
    }

    ++m_fpsFrames;
    const qint64 elapsed = m_fpsTimer.elapsed();
    if (elapsed > 500) {
        m_fps = m_fpsFrames * 1000.0 / static_cast<double>(elapsed);
        m_fpsFrames = 0;
        m_fpsTimer.restart();
        emit fpsChanged(m_fps);
    }

}

// ---------------------------------------------------------------------------
// input
// ---------------------------------------------------------------------------

void View::mousePressEvent(QMouseEvent *e)
{
    m_lastPos = e->position().toPoint();
    m_dragButton = e->button();
    e->accept();
}

void View::mouseMoveEvent(QMouseEvent *e)
{
    const QPoint pos = e->position().toPoint();
    const QPointF delta = pos - m_lastPos;
    m_lastPos = pos;

    switch (m_dragButton) {
    case Qt::LeftButton:
        orbit(delta);
        break;
    case Qt::RightButton:
        pan(delta);
        break;
    case Qt::MiddleButton:
        zoom(static_cast<float>(-delta.y()) * 0.05f);
        break;
    default:
        break;
    }
    e->accept();
}

void View::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragButton = Qt::NoButton;
    e->accept();
}

void View::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        showEntireScene();
        e->accept();
        return;
    }
    QOpenGLWidget::mouseDoubleClickEvent(e);
}

void View::wheelEvent(QWheelEvent *e)
{
    const QPoint angle = e->angleDelta();
    if (!angle.isNull()) {
        zoom(static_cast<float>(angle.y()) / 120.0f);
        e->accept();
        return;
    }
    QOpenGLWidget::wheelEvent(e);
}

void View::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_A:
        setAxisIsDrawn(!m_drawAxis);
        return;
    case Qt::Key_G:
        setGridIsDrawn(!m_drawGrid);
        return;
    case Qt::Key_R:
        resetCamView();
        return;
    case Qt::Key_Home:
    case Qt::Key_Space:
        showEntireScene();
        return;
    default:
        break;
    }
    // anything else belongs to the window's shortcuts
    QOpenGLWidget::keyPressEvent(e);
}
