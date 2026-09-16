#include "mainwin.h"
#include "ui_mainwin.h"

#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QLabel>
#include <QProcess>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QUrl>

using namespace Qt::StringLiterals;

MainWindow::MainWindow(QWidget *parent, bool fileMode, const QString &fileName)
    : QMainWindow(parent)
    , openFile(fileName)
    , bFileMode(fileMode)
    , ui(std::make_unique<Ui::MainWindow>())
{
    setAttribute(Qt::WA_QuitOnClose);

    ui->setupUi(this);

    view = new View(this);
    setCentralWidget(view);

    fpsLabel = new QLabel(this);
    fpsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    fpsLabel->setMinimumWidth(80);
    statusBar()->addPermanentWidget(fpsLabel);

    progressBar = new QProgressBar(this);
    progressBar->setMaximumWidth(120);
    progressBar->setTextVisible(false);
    progressBar->setRange(0, 0);
    progressBar->hide();
    statusBar()->addPermanentWidget(progressBar);

#if QT_CONFIG(process)
    commandProcess = new QProcess(this);
    commandProcess->setProcessChannelMode(QProcess::SeparateChannels);
#endif

    createG2mWorker();
    setupConnections();

    home_dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (!home_dir.endsWith(u'/'))
        home_dir += u'/';

    loadSettings();
    applyFontSize();
    updateSaveActions();

#if !QT_CONFIG(process)
    // the command pane runs a shell pipeline, and there is no shell to run it
    // in here, so the window is always laid out for viewing a file
    bFileMode = true;
#endif

    const bool commandMode = !bFileMode;
    ui->dockWidget->setHidden(bFileMode);
    ui->dockWidget_2->setHidden(bFileMode);

    if (commandMode) {
        connect(ui->command, &QPlainTextEdit::textChanged, this, &MainWindow::changedCommand);
        QTimer::singleShot(0, this, &MainWindow::loadSettingsCommand);
    } else {
        QTimer::singleShot(0, this, &MainWindow::loadGCodeFile);
    }
}

MainWindow::~MainWindow()
{
    if (g2mThread) { // null where the worker runs in the main thread
        g2mThread->quit();
        g2mThread->wait();
    }
}

void MainWindow::setupConnections()
{
    connect(ui->gcode, &QPlainTextEdit::textChanged, this, &MainWindow::changedGcode);

    connect(view, &View::fpsChanged, this, [this](double fps) {
        fpsLabel->setText(tr("%1 fps").arg(fps, 0, 'f', 1));
    });

    connect(ui->action_Quit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->action_showFullScreen, &QAction::triggered, this, &MainWindow::toggleFullScreen);
    connect(ui->action_Open, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(ui->action_Save, &QAction::triggered, this, &MainWindow::onSave);
    connect(ui->action_Save_As, &QAction::triggered, this, &MainWindow::onSaveAs);
    connect(ui->action_Settings, &QAction::triggered, this, [this] { onSettings(); });

    connect(ui->action_AutoZoom, &QAction::triggered, this, &MainWindow::toggleAutoZoom);
    connect(ui->actionZoom_In, &QAction::triggered, this, &MainWindow::zoomIn);
    connect(ui->actionZoom_out, &QAction::triggered, this, &MainWindow::zoomOut);
    connect(ui->action_Issues, &QAction::triggered, this, &MainWindow::helpIssues);
    connect(ui->action_Chat, &QAction::triggered, this, &MainWindow::helpChat);

#if QT_CONFIG(process)
    connect(commandProcess, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        if (commandPending) {
            runCommand();
            return;
        }
        // setPlainText emits textChanged, which is already wired to
        // changedGcode() - scheduling an interpreter run here would do it twice.
        ui->gcode->setPlainText(QString::fromUtf8(commandProcess->readAllStandardOutput()));
        ui->stderror->setPlainText(QString::fromUtf8(commandProcess->readAllStandardError()));
    });

    connect(commandProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::Crashed) // that is what the timeout looks like
            return;
        ui->stderror->setPlainText(commandProcess->errorString());
    });
#endif
}

void MainWindow::createG2mWorker()
{
#if QT_CONFIG(thread)
    g2mThread = new QThread(this);
    g2mWorker = new g2m::G2mWorker();
    g2mWorker->moveToThread(g2mThread);

    connect(g2mThread, &QThread::finished, g2mWorker, &QObject::deleteLater);
#else
    // A single-threaded build (Emscripten) has no second thread to move the
    // worker into, so it lives in the main one. The queued connections below
    // still do their job - they just come back round the only event loop
    // there is, which keeps the window painting between the two halves of an
    // interpreter run even though the run itself now blocks it.
    g2mWorker = new g2m::G2mWorker(this);
#endif

    connect(this, &MainWindow::setGcodeFile, g2mWorker, &g2m::G2mWorker::setFile, Qt::QueuedConnection);
    connect(this, &MainWindow::setToolTable, g2mWorker, &g2m::G2mWorker::setToolTable, Qt::QueuedConnection);
    connect(this, &MainWindow::interpret, g2mWorker, &g2m::G2mWorker::process,
            static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));

    connect(g2mWorker, &g2m::G2mWorker::signalNCend, view, &View::refresh, Qt::QueuedConnection);
    connect(g2mWorker, &g2m::G2mWorker::signalError, view, &View::refresh, Qt::QueuedConnection);
    connect(g2mWorker, &g2m::G2mWorker::signalCanonLines, view, &View::setCanonLines, Qt::QueuedConnection);
    connect(g2mWorker, &g2m::G2mWorker::debugMessage, this,
            [](const QString &msg) { qDebug() << "G2M:" << msg; });
    connect(g2mWorker, &g2m::G2mWorker::signalNCend, this, &MainWindow::hideProgressBar);
    connect(g2mWorker, &g2m::G2mWorker::signalError, this, &MainWindow::hideProgressBar);
    connect(g2mWorker, &g2m::G2mWorker::signalCanonLines, this, &MainWindow::hideProgressBar);
    connect(g2mWorker, &g2m::G2mWorker::signalError, this,
            [this](const QString &msg) { ui->stderror->setPlainText(msg); });

#if QT_CONFIG(thread)
    connect(g2mThread, &QThread::finished, this, &MainWindow::hideProgressBar);

    // Start it here: interpret() and the other worker slots are queued
    // connections, so anything emitted before the thread runs would just sit in
    // its event queue.
    g2mThread->start();
#endif
}

// ---------------------------------------------------------------------------
// view
// ---------------------------------------------------------------------------

void MainWindow::toggleAutoZoom()
{
    view->setAutoZoom(ui->action_AutoZoom->isChecked());
}

void MainWindow::toggleFullScreen()
{
    if (ui->action_showFullScreen->isChecked())
        showMaximized();
    else
        showNormal();
}

void MainWindow::zoomIn()
{
    ++fontSize;
    applyFontSize();
}

void MainWindow::zoomOut()
{
    fontSize = qMax(1, fontSize - 1);
    applyFontSize();
}

void MainWindow::applyFontSize()
{
    setStyleSheet(u"QWidget { font-size: %1pt; font-family: \"Courier\"; "
                  "background-color: #00003B; color: #FFA700; font: bold }"_s
                      .arg(fontSize));
}

// ---------------------------------------------------------------------------
// the command pane
// ---------------------------------------------------------------------------

void MainWindow::changedCommand()
{
    openFile.clear();
    setWindowTitle(u"QGCoder :- "_s);
    updateSaveActions();
    runCommand();
}

/// Run the command pane through a shell, asynchronously - a blocking
/// waitForFinished() here froze the whole window for as long as the pipeline
/// took. The script goes in on stdin, so there is no temporary file to create,
/// make executable and race someone else for.
void MainWindow::runCommand()
{
#if !QT_CONFIG(process)
    return; // no shell here; the command pane is hidden anyway
#else
    if (commandProcess->state() != QProcess::NotRunning) {
        // a newer edit supersedes the run in flight
        commandPending = true;
        commandProcess->kill();
        return;
    }

    commandPending = false;
    commandProcess->start(u"timeout"_s, {u"1"_s, u"bash"_s, u"-s"_s});
    commandProcess->write(ui->command->toPlainText().toUtf8());
    commandProcess->closeWriteChannel();
#endif
}

// ---------------------------------------------------------------------------
// the g-code pane
// ---------------------------------------------------------------------------

void MainWindow::changedGcode()
{
    // openInBrowser() fires textChanged once per appended line; interpreting on
    // each of those would mean one interpreter run per line of the file.
    if (bLoading)
        return;

    if (ui->gcode->toPlainText().isEmpty()) {
        view->clear();
        ui->stderror->clear();
        return;
    }

    QFile f(gcodefile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    QTextStream out(&f);
    out << ui->gcode->toPlainText();
    f.close();

    view->clear();

    emit setToolTable(tooltable);
    emit setGcodeFile(gcodefile);
    showProgressBar();
    emit interpret();
}

void MainWindow::loadGCodeFile()
{
#ifdef Q_OS_WASM
    // There is no command line in a browser to have named a file on, so fall
    // back to the sample compiled into the resources rather than opening on an
    // empty window. It has to be copied out first: the interpreter reads its
    // input through stdio, which knows nothing about qrc paths.
    if (openFile.isEmpty()) {
        const QString path = QDir::tempPath() + "/demo.ngc"_L1;
        if (QFile::exists(path) || QFile::copy(u":/doc/demo.ngc"_s, path)) {
            // a file copied out of the resources is read-only, and the editor
            // is meant to be able to save over this one
            QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            openFile = path;
        }
    }
#endif

    if (openFile.isEmpty())
        return;
    if (openInViewer(openFile) == 0)
        openInBrowser(openFile);
}

void MainWindow::onOpenFile()
{
#ifdef Q_OS_WASM
    // The browser hands us the contents rather than a path, and does it
    // asynchronously - there is no nested event loop to wait in. Drop what
    // comes back into the virtual file system so the rest of the code below
    // has the file it expects.
    QFileDialog::getOpenFileContent(
        tr("GCode Files (*.ngc *.nc);; All files (*.*)"),
        [this](const QString &name, const QByteArray &content) {
            if (name.isEmpty())
                return;
            const QString path = QDir::tempPath() + u'/' + QFileInfo(name).fileName();
            QFile f(path);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                ui->statusbar->showMessage(tr("Error loading file %1").arg(name), 5000);
                return;
            }
            f.write(content);
            f.close();
            if (openInViewer(path) == 0)
                openInBrowser(path);
        },
        this);
#else
    const QString filename =
        QFileDialog::getOpenFileName(this, tr("Open G-code"), home_dir + "machinekit"_L1,
                                     tr("GCode Files (*.ngc *.nc);; All files (*.*)"));
    if (filename.isEmpty())
        return;
    if (openInViewer(filename) == 0)
        openInBrowser(filename);
#endif
}

int MainWindow::openInViewer(const QString &filename)
{
    QFile fin(filename);
    QFile fout(gcodefile);

    if (!fin.open(QFile::ReadOnly | QFile::Text)
        || !fout.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return -1;

    QTextStream ints(&fin);
    QTextStream outts(&fout);
    while (!ints.atEnd())
        outts << ints.readLine() << '\n';
    fin.close();
    fout.close();

    view->clear();

    emit setToolTable(tooltable);
    emit setGcodeFile(gcodefile);
    showProgressBar();
    emit interpret();

    return 0;
}

void MainWindow::openInBrowser(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        ui->statusbar->showMessage(tr("Error loading file %1").arg(filename), 5000);
        return;
    }

    ui->statusbar->showMessage(tr("Loading file %1").arg(filename), 5000);

    bLoading = true;
    ui->gcode->clear();

    QTextStream ts(&file);
    while (!ts.atEnd())
        ui->gcode->appendNewPlainText(ts.readLine());
    bLoading = false;
    file.close();

    setWindowTitle(u"QGCoder :- "_s + filename);

    openFile = filename;
    updateSaveActions();
}

/// Save over the file the editor is showing.
void MainWindow::onSave()
{
#ifdef Q_OS_WASM
    // The file system here is the one Emscripten keeps in memory, so writing
    // back to the open file would leave the result somewhere the user cannot
    // get at. Hand it to the browser's download machinery instead.
    onSaveAs();
#else
    // Nothing loaded - the command pane feeds the editor, or this is a fresh
    // window - so there is no path to write to yet. Ask for one.
    if (openFile.isEmpty()) {
        onSaveAs();
        return;
    }

    saveInBrowser(openFile);
#endif
}

void MainWindow::onSaveAs()
{
#ifdef Q_OS_WASM
    // "Save as" in a browser means handing the bytes to the download machinery
    // under a suggested name; where they end up is the browser's business.
    const QString hint = openFile.isEmpty() ? u"untitled.ngc"_s : QFileInfo(openFile).fileName();
    QFileDialog::saveFileContent(ui->gcode->toPlainText().toUtf8(), hint, this);
#else
    const QString fileName =
        QFileDialog::getSaveFileName(this, tr("Save G-code (As)"), openFile,
                                     tr("G-code Files (*.ngc *.nc);; All files (*.*)"));
    if (!fileName.isEmpty())
        saveInBrowser(fileName);
#endif
}

int MainWindow::saveInBrowser(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        ui->statusbar->showMessage(tr("Error saving file %1").arg(filename), 5000);
        return -1;
    }

    QTextStream out(&file);
    out << ui->gcode->toPlainText();
    file.close();

    ui->statusbar->showMessage(tr("Saved %1").arg(filename), 5000);
    setWindowTitle(u"QGCoder :- "_s + filename);

    // "Save as" used to leave openFile on the file that had been *opened*, so
    // the title named one file and the next save wrote to another.
    openFile = filename;
    updateSaveActions();
    return 0;
}

/// Both save actions name the file they would write to, so they have to be
/// re-labelled whenever that changes.
void MainWindow::updateSaveActions()
{
    if (openFile.isEmpty()) {
        ui->action_Save->setText(tr("&Save"));
        ui->action_Save_As->setText(tr("&Save &As..."));
        return;
    }

    const QString name = QFileInfo(openFile).fileName();
    ui->action_Save->setText(tr("&Save \"%1\"").arg(name));
    ui->action_Save_As->setText(tr("&Save \"%1\" &As...").arg(name));
}

// ---------------------------------------------------------------------------
// settings
// ---------------------------------------------------------------------------

void MainWindow::loadSettings()
{
    settings.beginGroup(u"gui"_s);

    restoreGeometry(settings.value(u"geometry"_s, saveGeometry()).toByteArray());
    restoreState(settings.value(u"state"_s, saveState()).toByteArray());
    move(settings.value(u"pos"_s, pos()).toPoint());
    resize(settings.value(u"size"_s, size()).toSize());

    const bool maximized = settings.value(u"maximized"_s, isMaximized()).toBool();
    if (maximized)
        showMaximized();
    ui->action_showFullScreen->setChecked(maximized);

    const bool autoZoom = settings.value(u"autoZoom"_s, view->autoZoom()).toBool();
    view->setAutoZoom(autoZoom);
    ui->action_AutoZoom->setChecked(autoZoom);

    fontSize = settings.value(u"fontsize"_s, 12).toInt();

    tooltable = settings.value(u"tooltable"_s).toString();
    gcodefile = settings.value(u"gcodefile"_s).toString();

    settings.endGroup();

#ifdef Q_OS_WASM
    // Insisting here the way the desktop build does would mean a nested event
    // loop, which the browser build has not got. There is nowhere else to put
    // the scratch file anyway: the file system is the one Emscripten keeps in
    // memory, so just pick a name in it.
    if (gcodefile.isEmpty())
        gcodefile = QDir::tempPath() + "/qgcoder-scratch.ngc"_L1;
#else
    // without a scratch g-code file we cannot work properly, so insist
    while (gcodefile.isEmpty()) {
        if (onSettings() == 0)
            break;
    }
#endif
}

void MainWindow::loadSettingsCommand()
{
    settings.beginGroup(u"gui"_s);
    ui->command->document()->setPlainText(
        settings.value(u"command"_s, u"/bin/echo -en 'Hello, World!' | hf2gcode"_s).toString());
    settings.endGroup();
}

void MainWindow::saveSettings()
{
    settings.beginGroup(u"gui"_s);

    settings.setValue(u"geometry"_s, saveGeometry());
    settings.setValue(u"state"_s, saveState());
    settings.setValue(u"maximized"_s, isMaximized());

    if (!isMaximized()) {
        settings.setValue(u"pos"_s, pos());
        settings.setValue(u"size"_s, size());
    }

    settings.setValue(u"command"_s, ui->command->toPlainText());
    settings.setValue(u"autoZoom"_s, ui->action_AutoZoom->isChecked());
    settings.setValue(u"fontsize"_s, fontSize);
    settings.setValue(u"tooltable"_s, tooltable);
    settings.setValue(u"gcodefile"_s, gcodefile);

    settings.endGroup();
}

int MainWindow::onSettings()
{
#ifdef Q_OS_WASM
    // exec() would spin a nested event loop; open() shows the dialog and
    // returns, so the values have to be collected when it is accepted.
    auto *dlg = new SettingsDialog(this, home_dir);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setValues(tooltable, gcodefile);
    connect(dlg, &QDialog::accepted, this, [this, dlg] {
        tooltable = dlg->tooltable;
        gcodefile = dlg->gcodefile;
    });
    dlg->open();
#else
    SettingsDialog dlg(this, home_dir);
    dlg.setValues(tooltable, gcodefile);

    if (dlg.exec() == QDialog::Accepted) {
        tooltable = dlg.tooltable;
        gcodefile = dlg.gcodefile;
    }
#endif

    return gcodefile.isEmpty() ? 1 : 0;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

// ---------------------------------------------------------------------------
// misc
// ---------------------------------------------------------------------------

void MainWindow::helpIssues()
{
    QDesktopServices::openUrl(QUrl(u"https://github.com/QGCoder/qgcoder/issues"_s));
}

void MainWindow::helpChat()
{
    QDesktopServices::openUrl(QUrl(u"https://gitter.im/QGCoder/qgcoder"_s));
}

void MainWindow::showProgressBar()
{
    progressBar->show();
}

void MainWindow::hideProgressBar()
{
    progressBar->hide();
}
