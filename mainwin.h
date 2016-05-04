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

// max file size of 25KB
#define MAX_SIZE 25000
// render initial 3K chunk
#define CHUNK_SIZE 3000
// add in 1K bits dynamically
#define ADD_SIZE 1000

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

    void onOpenFile();
    void onSaveAs();
    int onSettings();
    void onChangedPosition();

    void appendCanonLine(g2m::canonLine*);

    void toggleAutoZoom();

    void helpDonate();
    void helpIssues();
    void helpChat();

protected:
    void loadSettings();
    void saveSettings();

private:
    int openInViewer(QString filename);
    void openInBrowser(QString filename);
    int reOpenInBrowser();
    int saveInBrowser(QString& filename);

    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE;

private:
    QString home_dir, openFile;
    int fileSize, filePos;
    bool bMoreBig, bBigFile, bFileMode, bFirstCallDone;

    QString rs274;
    QString tooltable;
    QString gcodefile;

    Ui::MainWindow *ui;

    View *view;

    g2m::g2m *g2m;

    QSettings *settings;
};

#endif // MAINWINDOW_H
