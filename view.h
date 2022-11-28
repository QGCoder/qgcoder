#ifndef VIEW_H
#define VIEW_H

#include <QGLViewer/qglviewer.h>
#include <QGLViewer/manipulatedFrame.h>

#include <QMutex>
#include <QSettings>

#include "g2m.hpp"

using namespace qglviewer;
using namespace g2m;

class View : public QGLViewer
{
  Q_OBJECT;
  Q_PROPERTY(bool autoZoom READ autoZoom WRITE setAutoZoom RESET unsetAutoZoom);

 public:
  View(QWidget *parent = NULL);
  ~View();

  void resetCamView();

  void updateGLViewer() {
#if QGLVIEWER_VERSION < 0x020700
      this->updateGL();
#else
      this->update();
#endif
  };

  bool autoZoom()const{ return _autoZoom; };
  void setAutoZoom(bool autoZoom){ _autoZoom = autoZoom; update(); };
  void unsetAutoZoom(){ _autoZoom = true; update(); };

 public slots:
  void close();

  void appendCanonLine(canonLine *l);
  void clear();

  void update();

  void keyPressEvent(QKeyEvent *e);

 protected:
  void init();
  void initializeGL();
      
  virtual void draw();
  virtual void fastDraw();
  virtual void postDraw();

  void drawObjects(bool simplified);

 private:
  Vec initialCameraPosition;
  Quaternion initialCameraOrientation;

  QMutex mutex;

  QSettings * settings;

  std::vector<g2m::canonLine*> lines;

  bool dirty;

  double aabb[6];

  bool _autoZoom;
};

#endif // VIEW_H

