#include "QCLIProcess.h"

#include <QtCore>

QCLIProcess::QCLIProcess(QObject *parent) :
    QProcess(parent)
{
    setProcessChannelMode(QProcess::SeparateChannels);

    connect((QProcess *)this, SIGNAL(readyReadStandardOutput()), this, SLOT(processStdoutLine()));
    connect((QProcess *)this, SIGNAL(readyReadStandardError()),  this, SLOT(processStderrLine()));
}

void QCLIProcess::processStdoutLine() {
    stdout.append(readAllStandardOutput());

    int last = 0;
    for(int i=0; i < stdout.size(); i++){
        if (stdout.at(i) == '\n') {
            QString line(stdout.mid(last, i-last));
            lstdout << line;
            last = i+1;
        }
    }

    stdout.remove(0, last);
    emit readyReadStdout();
}

QString QCLIProcess::readLineStdout() {
    QString line;

    if (!lstdout.isEmpty()){
        line = lstdout.at(0);
        lstdout.removeFirst();
    }

    return line;
}

void QCLIProcess::processStderrLine() {
    stderr.append(readAllStandardError());

    int last = 0;
    for(int i=0; i<stderr.size(); i++){
        if (stderr.at(i) == '\n'){
            QString line(stderr.mid(last, i-last));
            lstderr << line;
            last = i+1;
        }
    }

    stderr.remove(0, last);
    emit readyReadStderr();
}

bool QCLIProcess::canReadLineStderr() const {
    return !lstderr.isEmpty();
}

QString QCLIProcess::readLineStderr() {
    QString line;

    if (!lstderr.isEmpty()){
        line = lstderr.at(0);
        lstderr.removeFirst();
    }

    return line;
}
