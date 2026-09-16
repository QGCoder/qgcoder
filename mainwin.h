#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QSettings>
#include <QString>

#include <memory>

#include <QGCodeEditor/QGCodeEditor.h>

#include "g2m/g2mworker.h"
#include "settings_dlg.h"
#include "view.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QProcess;
class QThread;
QT_END_NAMESPACE

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, bool fileMode = false, const QString &fileName = {});
    ~MainWindow() override;

public slots:
    /// load the last command string from the settings; used during startup
    void loadSettingsCommand();
    void loadGCodeFile();

    void changedGcode();
    void changedCommand();

    void showProgressBar();
    void hideProgressBar();

    void onOpenFile();
    void onSaveAs();
    int onSettings();

    void toggleAutoZoom();
    void toggleFullScreen();
    void zoomIn();
    void zoomOut();

    void helpIssues();
    void helpChat();

signals:
    void setToolTable(const QString &s);
    void setGcodeFile(const QString &f);
    void interpret();

protected:
    void closeEvent(QCloseEvent *event) override;

private: // functions
    void loadSettings();
    void saveSettings();

    int openInViewer(const QString &filename);
    void openInBrowser(const QString &filename);
    int saveInBrowser(const QString &filename);

    void setupConnections();
    void applyFontSize();
    void createG2mWorker();
    void runCommand();

private: // data
    QString home_dir;
    QString openFile;
    /// started from the command line with a g-code file: lays the window out
    /// for viewing a file rather than driving a command
    bool bFileMode = false;
    /// set while openInBrowser() fills the editor, to suppress changedGcode()
    bool bLoading = false;

    QString tooltable;
    QString gcodefile;

    std::unique_ptr<Ui::MainWindow> ui;

    View *view = nullptr;

    g2m::G2mWorker *g2mWorker = nullptr;
    QThread *g2mThread = nullptr;

    QProgressBar *progressBar = nullptr;
    /// render rate of the 3D view, shown in the status bar
    QLabel *fpsLabel = nullptr;
    /// the shell pipeline behind the command pane, run without blocking the GUI
    QProcess *commandProcess = nullptr;
    /// a command edit that arrived while the previous one was still running
    bool commandPending = false;

    int fontSize = 12;

    QSettings settings;
};

#endif // MAINWINDOW_H
