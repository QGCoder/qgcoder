/// \file
/// The main window: the editor panes, the 3D view and the menu.

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

/// \brief The application window.
///
/// Holds the g-code editor, the 3D tool-path view and, in command mode, a
/// pane whose shell pipeline generates the g-code. Editing either pane writes
/// the scratch file and asks the worker - which lives on its own thread - to
/// interpret it again, so the view follows the text as it is typed.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// \param parent   widget parent
    /// \param fileMode true to open a file, false for the command pane
    /// \param fileName the g-code file to open in file mode
    explicit MainWindow(QWidget *parent = nullptr, bool fileMode = false, const QString &fileName = {});
    /// Stops the interpreter thread and waits for it.
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
    void onSave();
    void onSaveAs();
    int onSettings();

    void toggleAutoZoom();
    void toggleFullScreen();
    void zoomIn();
    void zoomOut();

    void helpIssues();
    void helpChat();

signals:
    /// tell the worker which tool table to use \param s the path, may be empty
    void setToolTable(const QString &s);
    /// tell the worker which file to read \param f the scratch file path
    void setGcodeFile(const QString &f);
    /// ask the worker for another interpretation
    void interpret();

protected:
    void closeEvent(QCloseEvent *event) override;

private: // functions
    void loadSettings();
    void saveSettings();

    QString defaultToolTable();

    int openInViewer(const QString &filename);
    void openInBrowser(const QString &filename);
    int saveInBrowser(const QString &filename);

    void updateSaveActions();
    void setupConnections();
    void applyFontSize();
    void createG2mWorker();
    void runCommand();

private: // data
    QString home_dir;       ///< where the file dialogs start, trailing '/'
    QString openFile;       ///< the file the editor is showing, if any
    /// started from the command line with a g-code file: lays the window out
    /// for viewing a file rather than driving a command
    bool bFileMode = false;
    /// set while openInBrowser() fills the editor, to suppress changedGcode()
    bool bLoading = false;

    QString tooltable;      ///< tool table path; empty means the default
    QString gcodefile;      ///< scratch file the interpreter reads

    std::unique_ptr<Ui::MainWindow> ui;  ///< the widgets, from mainwin.ui

    View *view = nullptr;   ///< the 3D tool-path view, the central widget

    g2m::G2mWorker *g2mWorker = nullptr;  ///< interpreter, on g2mThread
    QThread *g2mThread = nullptr;         ///< the thread it lives on

    QProgressBar *progressBar = nullptr;  ///< busy indicator while running
    /// render rate of the 3D view, shown in the status bar
    QLabel *fpsLabel = nullptr;
#if QT_CONFIG(process)
    /// the shell pipeline behind the command pane, run without blocking the GUI
    QProcess *commandProcess = nullptr;
#endif
    /// a command edit that arrived while the previous one was still running
    bool commandPending = false;

    int fontSize = 12;      ///< point size of the text panes

    QSettings settings;     ///< geometry, paths and the command pane
};

#endif // MAINWINDOW_H
