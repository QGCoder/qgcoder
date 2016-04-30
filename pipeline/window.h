#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "pipe.h"

namespace Ui {
class MainWindow;
}

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(Pipe *p, QWidget *parent = 0);
    ~Window();

public slots:
    void appendGCode(QString gCode);

private:
    Ui::MainWindow *ui;

    Pipe *p;
};

#endif // MAINWINDOW_H
