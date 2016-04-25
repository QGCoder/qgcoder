#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QSettings>

#include "QGCodeEditor.h"
#include "view.h"
#include "g2m.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void parseGcode();
    void parseCommand();

signals:
    void setRS274(QString s);
    void setToolTable(QString s);
    void setGcodeFile(QString f);
    void interpret();

public slots:
    // load the last command string from the settings
    // used during startup
    void loadSettingsCommand();

    void changedGcode();
    void changedCommand();

    void appendCanonLine(g2m::canonLine*);

    void toggleAutoZoom();

    void helpDonate();
    void helpIssues();
    void helpChat();

protected:
    void loadSettings();
    void saveSettings();

private:
    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE;

private:
    Ui::MainWindow *ui;

    View *view;

    g2m::g2m *g2m;

    QSettings *settings;
};

#endif // MAINWINDOW_H
