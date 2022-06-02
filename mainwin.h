#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QSettings>

#include "QGCodeEditor.h"
#include "view.h"
#include "g2m.hpp"
#include "settings_dlg.h"

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
    virtual void loadSettingsCommand();

    virtual void changedGcode();
    virtual void changedCommand();

    virtual void onOpenFile();
    virtual void onSaveAs();
    virtual int onSettings();

    virtual void appendCanonLine(g2m::canonLine*);

    virtual void toggleAutoZoom();
    virtual void showFullScreen();
    virtual void zoomIn();
    virtual void zoomOut();

    virtual void helpIssues();
    virtual void helpChat();

protected:
    void loadSettings();
    void saveSettings();

private:  // functions
    int openInViewer(QString filename);
    void openInBrowser(QString filename);
    int saveInBrowser(QString& filename);

    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE;

    void setStyle();

private: // data
    QString home_dir, openFile;
    bool bFileMode;

    QString rs274;
    QString tooltable;
    QString gcodefile;

    Ui::MainWindow *ui;

    View *view;

    g2m::g2m *g2m;

    int fontSize;

    QSettings *settings;
};

#endif // MAINWINDOW_H
