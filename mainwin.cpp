#include "mainwin.h"
#include "ui_mainwin.h"

#include <QPushButton>
#include <QProcess>
#include <QDesktopServices>

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
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
    connect(ui->command, SIGNAL(textChanged()), this, SLOT(changedCommand()));

    connect(ui->action_AutoZoom, SIGNAL(triggered()), this, SLOT(toggleAutoZoom()));
    
    //connect(ui->command, SIGNAL(keyPressed(QKeyEvent *)), view, SLOT(keyPressEvent(QKeyEvent *)));

    connect(ui->action_Donate, SIGNAL(triggered(bool)), this, SLOT(helpDonate()));
    connect(ui->action_Issues, SIGNAL(triggered(bool)), this, SLOT(helpIssues()));
    connect(ui->action_Chat,   SIGNAL(triggered(bool)), this, SLOT(helpChat()));

    loadSettings();

    QTimer::singleShot(0, this, SLOT(loadSettingsCommand()));
}

MainWindow::~MainWindow()
{
//    delete ui;
//    delete settings;
//    delete g2m;
}

void MainWindow::toggleAutoZoom() {
    view->setAutoZoom(ui->action_AutoZoom->isChecked());
}

void MainWindow::changedCommand() {
    parseCommand();
}

void MainWindow::changedGcode() {

    if (ui->gcode->toPlainText().isEmpty()) {
        view->clear();
        ui->stderror->setPlainText("");
        return;
    }

    QFile f( "/tmp/gcode.ngc" );
    if ( f.open( QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text) ) {
        QTextStream out(&f);
        out << ui->gcode->toPlainText();
        f.close();
    
        view->clear();
    
        emit setRS274("/usr/src/machinekit-main/bin/rs274");
        emit setToolTable("/usr/src/machinekit-main/configs/sim/axis/sim_mm.tbl");
        emit setGcodeFile("/tmp/gcode.ngc");
        
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

void MainWindow::loadSettings() {
  settings->beginGroup("gui");

  restoreGeometry(settings->value("geometry", saveGeometry() ).toByteArray());
  restoreState(settings->value("state", saveState() ).toByteArray());
  move(settings->value("pos", pos()).toPoint());
  resize(settings->value("size", size()).toSize());

  if (settings->value("maximized", isMaximized() ).toBool()) {
    showMaximized();
  }

  view->setAutoZoom(settings->value("autoZoom", view->autoZoom()).toBool());
  ui->action_AutoZoom->setChecked(settings->value("autoZoom", view->autoZoom()).toBool());

  settings->endGroup();
}

void MainWindow::loadSettingsCommand() {
    QString command("echo -n 'Hello, World!' | hf2gcode -q");
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
  
  settings->endGroup();
}

void MainWindow::closeEvent(QCloseEvent * event) {
  qDebug() << "MainWindow::closeEvent";
  saveSettings();
//      ui.viewer->close();
  event->accept();

  QMainWindow::closeEvent(event);
}

void MainWindow::helpDonate() {
    QDesktopServices::openUrl(QUrl("https://koppi.github.com/paypal"));
}

void MainWindow::helpIssues() {
    QDesktopServices::openUrl(QUrl("https://github.com/koppi/gcoder/issues"));
}

void MainWindow::helpChat() {
    QDesktopServices::openUrl(QUrl("https://gitter.im/koppi/gcoder"));
}
