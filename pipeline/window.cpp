#include "window.h"
#include "ui_window.h"

#include <QDebug>

Window::Window(Pipe *pipe, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    p = pipe;

    ui->gcodeEditor->clear();
    ui->gcodeEditor->document()->setMaximumBlockCount(20);

    connect(p, SIGNAL(signalBash2StandardOutput(QString)),
            this, SLOT(appendGCode(QString)));
}

Window::~Window()
{
    delete ui;
}

void Window::appendGCode(QString gCode) {
    //qDebug() << gCode;
    QString prompt = "READ => ";

    if (gCode.startsWith(prompt)) {
        QString input = gCode.mid(prompt.length());
        ui->gcodeEditor->appendPlainText(input);
    }
}
