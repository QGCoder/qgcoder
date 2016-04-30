#ifndef QCLIPROCESS_H
#define QCLIPROCESS_H

#include <QProcess>
#include <QStringList>
#include <QByteArray>

class QCLIProcess : public QProcess
{
    Q_OBJECT

public:
    explicit QCLIProcess(QObject *parent = 0);

    bool canReadLineStdout() const;
    QString readLineStdout();

    bool canReadLineStderr() const;
    QString readLineStderr();

signals:
    void readyReadStdout();
    void readyReadStderr();

private slots:
    void processStdoutLine();
    void processStderrLine();

private:
    QByteArray stdout;
    QByteArray stderr;
    QStringList lstdout;
    QStringList lstderr;
};

#endif // QCLIPROCESS_H
