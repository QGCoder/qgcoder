#include "mainwin.h"
#include "ui_mainwin.h"

#include <QPushButton>
#include <QProcess>
#include <QDesktopServices>
#include <QFileDialog>
#include <QDebug>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent, bool fileMode, QString fileName) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    bFileMode = fileMode;

    setAttribute(Qt::WA_QuitOnClose);

    ui->setupUi(this);

    settings = new QSettings();

    view = new View(this);

    setCentralWidget(view);

    g2m = new g2m::g2m(); // g-code interpreter

    connect( this, SIGNAL( setGcodeFile(QString) ),     g2m, SLOT( setFile(QString)) );
    connect( this, SIGNAL( setRS274(QString) ),         g2m, SLOT( setInterp(QString)) );
    connect( this, SIGNAL( setToolTable(QString) ),     g2m, SLOT( setToolTable(QString)) );
    connect( this, SIGNAL( interpret() ),               g2m, SLOT( interpret_file() ) );

    //connect( g2m, SIGNAL( debugMessage(QString) ),     this, SLOT( debugMessage(QString) ) );
    //connect( g2m, SIGNAL( gcodeLineMessage(QString) ), this, SLOT( appendGcodeLine(QString) ) );
    //connect( g2m, SIGNAL( canonLineMessage(QString) ), this, SLOT( appendCanonLine(QString) ) );

    connect( g2m, SIGNAL( signalCanonLine(canonLine*) ), view, SLOT( appendCanonLine(canonLine*) ));
    connect( g2m, SIGNAL( signalNCend() ),               view, SLOT( update() ) );
    connect( g2m, SIGNAL( signalError(QString) ),        view, SLOT( update() ) );

    connect( g2m, SIGNAL( signalError(QString) ),        ui->stderror, SLOT( setPlainText(QString) ) );

    connect(ui->gcode, SIGNAL(textChanged()), this, SLOT(changedGcode()));

    connect(ui->action_AutoZoom, SIGNAL(triggered()), this, SLOT(toggleAutoZoom()));
    connect(ui->actionZoom_In, SIGNAL(triggered()), this, SLOT(zoomIn()));
    connect(ui->actionZoom_out, SIGNAL(triggered()), this, SLOT(zoomOut()));

    //connect(ui->command, SIGNAL(keyPressed(QKeyEvent *)), view, SLOT(keyPressEvent(QKeyEvent *)));

    connect(ui->action_Issues, SIGNAL(triggered(bool)), this, SLOT(helpIssues()));
    connect(ui->action_Chat,   SIGNAL(triggered(bool)), this, SLOT(helpChat()));

    home_dir = QStandardPaths::locate(QStandardPaths::HomeLocation, QString(), QStandardPaths::LocateDirectory);
    openFile = fileName;

    loadSettings();

    setStyle();

    if (bFileMode == false) {
        connect(ui->command, SIGNAL(textChanged()), this, SLOT(changedCommand()));
        QTimer::singleShot(0, this, SLOT(loadSettingsCommand()));
        ui->dockWidget->setHidden(false);
        ui->dockWidget_2->setHidden(false);
    } else {
        QTimer::singleShot(0, this, SLOT(loadGCodeFile()));
        ui->dockWidget->setHidden(true);
        ui->dockWidget_2->setHidden(true);
    }
}

MainWindow::~MainWindow()
{

}

void MainWindow::loadGCodeFile() {
    if(openFile.length())
        {
            if(openInViewer(openFile) == 0) {
                openInBrowser(openFile);
            }
        }
}

void MainWindow::toggleAutoZoom() {
    view->setAutoZoom(ui->action_AutoZoom->isChecked());
}

void MainWindow::showFullScreen()
{
    if( ui->action_showFullScreen->isChecked() )
        showMaximized();
    else
        showNormal();
}

void MainWindow::zoomIn() {
    fontSize += 1;
    setStyle();
}

void MainWindow::zoomOut() {
    fontSize -= 1; if (fontSize <1) fontSize = 1;
    setStyle();
}

void MainWindow::setStyle() {
    setStyleSheet(QString("QWidget { font-size: %1pt; font-family: \"Courier\"; background-color: #00003B; color: #FFA700; font: bold }").arg(fontSize));
}

void MainWindow::changedCommand()
{
QString str;

    openFile = "";
    bFileMode = false;
    connect(ui->gcode, SIGNAL(textChanged()), this, SLOT(changedGcode()));
    str = "QGCoder :- ";
    setWindowTitle(str);
    parseCommand();
}

void MainWindow::changedGcode() {

    if(bFileMode)
        return;

    if (ui->gcode->toPlainText().isEmpty()) 
        {
        view->clear();
        ui->stderror->setPlainText("");
        return;
        }

    QFile f(gcodefile);
    if ( f.open( QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text) ) 
        {
        QTextStream out(&f);
        out << ui->gcode->toPlainText();
        f.close();

        view->clear();

        emit setRS274(rs274);
        emit setToolTable(tooltable);
        emit setGcodeFile(gcodefile);

        emit interpret();
        }
}

void MainWindow::appendCanonLine(g2m::canonLine* l) {
    view->appendCanonLine(l);
}

void MainWindow::parseCommand() {
    QProcess sh;
    //sh.setProcessChannelMode(QProcess::MergedChannels);
    sh.setProcessChannelMode(QProcess::SeparateChannels);
    
    QFile f( "/tmp/gcoder.sh" );
    if ( f.open( QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text) ) {
        QTextStream out(&f);
        out << ui->command->toPlainText();
        f.close();
    }

    if (!f.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther)) {
        qDebug("XXX");
    }

    sh.start("bash", QStringList() << "-c" << "timeout 1 /tmp/gcoder.sh");

    if (!sh.waitForStarted()) {
        sh.close();
        //sh.kill();
        sh.waitForFinished(-1);
        return; // report error
    }

    if (!sh.waitForFinished(-1)) {
        // XXX http://stackoverflow.com/questions/10777147/terminate-an-ongoing-qprocess-that-is-running-inside-a-qthread
    }

    ui->gcode->setPlainText(sh.readAllStandardOutput());
    ui->stderror->setPlainText(sh.readAllStandardError());

    sh.close();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void MainWindow::loadSettings() 
{

    settings->beginGroup("gui");

    restoreGeometry(settings->value("geometry", saveGeometry() ).toByteArray());
    restoreState(settings->value("state", saveState() ).toByteArray());
    move(settings->value("pos", pos()).toPoint());
    resize(settings->value("size", size()).toSize());

    if (settings->value("maximized", isMaximized() ).toBool()) 
        showMaximized();
    ui->action_showFullScreen->setChecked(settings->value("maximized", isMaximized() ).toBool());
    
    view->setAutoZoom(settings->value("autoZoom", view->autoZoom()).toBool());
    ui->action_AutoZoom->setChecked(settings->value("autoZoom", view->autoZoom()).toBool());

    fontSize = settings->value("fontsize", 12).toInt();

    rs274 = settings->value("rs274", "").toString();
    tooltable = settings->value("tooltable", "").toString();
    gcodefile = settings->value("gcodefile", "").toString();

    settings->endGroup();
        // if any of these paths is not known, cannot work properly, so insist
    if(rs274.isEmpty() || tooltable.isEmpty() || gcodefile.isEmpty())
        {
        int ret = 1;
        while(ret)
            ret = onSettings();
        }
}

void MainWindow::loadSettingsCommand() {
    QString command("/bin/echo -en 'Hello, World!' | hf2gcode");
    settings->beginGroup("gui");
    ui->command->document()->setPlainText(settings->value("command", command).toString());
    settings->endGroup();
}

void MainWindow::saveSettings() {
  settings->beginGroup("gui");

  settings->setValue("geometry", saveGeometry());
  settings->setValue("state", saveState());
  settings->setValue("maximized", isMaximized());

  if ( !isMaximized() ) {
    settings->setValue("pos", pos());
    settings->setValue("size", size());
  }

  settings->setValue("command", ui->command->toPlainText());

  settings->setValue("autoZoom", ui->action_AutoZoom->isChecked());
  settings->setValue("fontsize", fontSize);

  settings->setValue("rs274", rs274);
  settings->setValue("tooltable", tooltable);
  settings->setValue("gcodefile", gcodefile);
  
  settings->endGroup();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void MainWindow::closeEvent(QCloseEvent * event) 
{
//  qDebug() << "MainWindow::closeEvent";
  saveSettings();
//      ui.viewer->close();

    //  we are leaving it to QMainwindow to decide if to accept
//  event->accept();

  QMainWindow::closeEvent(event);
}

////////////////////////////////////////////////////////////////////////////////////////////////

void MainWindow::onOpenFile()
{
QString filename;
QString path = home_dir + "machinekit";

    filename = QFileDialog::getOpenFileName(this, tr("Open G-code"), path, tr("GCode Files (*.ngc *.nc);; All files (*.*)"));
    if(filename.length())
        {
        if(openInViewer(filename) == 0)
            openInBrowser(filename);
        }
}

int MainWindow::openInViewer(QString filename)
{
QFile fin(filename);
QFile fout(gcodefile);
QString str;

    if (fin.open(QFile::ReadOnly | QFile::Text) &&  ( fout.open( QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text) ) )
        {
        QTextStream ints(&fin);
        QTextStream outts(&fout);
        while(!ints.atEnd())
            outts <<  ints.readLine() << "\n";
        fin.close();
        fout.close();

        view->clear();
        bFileMode = true;

        emit setRS274(rs274);
        emit setToolTable(tooltable);
        emit setGcodeFile(gcodefile);

        emit interpret();
        return 0;
        }
    else 
        return -1;
}

void MainWindow::openInBrowser(QString filename)
{
QFile file(filename);
QString str;

    if (file.open(QFile::ReadOnly | QFile::Text))
        {
        str = "Loading file " + filename;
        ui->statusbar->showMessage(str, 5000);
        ui->gcode->clear();

        QTextStream ts(&file);

        while( !ts.atEnd())
            {
            str = ts.readLine();
            ui->gcode->appendNewPlainText(str);
            }
        file.close();  

        str = "QGCoder :- " +  filename;
        setWindowTitle(str);

        // ui->gcode->highlightLine(1);

        openFile = filename;
        bFileMode = true;
        }
    else
        {
        str = "Error loading file " + filename ;
        ui->statusbar->showMessage(str, 5000);
        }
}


////////////////////////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSaveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save G-code (As)"), openFile, tr("G-code Files (*.ngc *.nc);; All files (*.*)"));
    saveInBrowser(fileName);
}



int  MainWindow::saveInBrowser(QString& filename)
{
QFile file(filename);
QString str;

    if (file.open(QIODevice::ReadWrite | QIODevice::Text))
        {
        QTextStream out(&file);
        out << ui->gcode->toPlainText();
        file.close();

        str = "QGCoder:- " +  filename;
        setWindowTitle(str);
        return 0;
        }
    else
        {
        str = "Error saving file " + filename ;
        ui->statusbar->showMessage(str, 5000);
        return -1;
        }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

int MainWindow::onSettings()
{

    SettingsDialog *dlg = new SettingsDialog(this, home_dir);

    dlg->setValues(rs274, tooltable, gcodefile);

    dlg->exec();

    if(dlg->result())
        {
        rs274 = dlg->rs274;
        tooltable = dlg->tooltable;
        gcodefile = dlg->gcodefile;
        }

    if(rs274.isEmpty() || tooltable.isEmpty() || gcodefile.isEmpty())	
        return 1;
    else
        return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void MainWindow::helpIssues() {
    QDesktopServices::openUrl(QUrl("https://github.com/QGCoder/qgcoder/issues"));
}

void MainWindow::helpChat() {
    QDesktopServices::openUrl(QUrl("https://gitter.im/QGCoder/qgcoder"));
}
