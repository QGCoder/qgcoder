#ifndef G2MWORKER_H
#define G2MWORKER_H

#include <QThread>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QVector>
#include <QDebug>

#include "g2m.hpp"
#include "nanotimer.hpp"
#include "machineStatus.hpp"

namespace g2m {

class G2mWorker : public QThread
{
    Q_OBJECT

public:
    G2mWorker(QObject *parent = nullptr) : QThread(parent) {
        QObject::setObjectName("G2mWorker");
    }

    void setFile(QString f) { file = f; }
    void setToolTable(QString tbl) { tooltable = tbl; }
    void setInterp(QString interp_binary) { interp = interp_binary; }

public slots:
    void stop() { interrupted = true; }
    void process() { 
        qDebug() << "G2mWorker::process() called"; 
        interpret_file_async(); 
    }
    void quitAndDelete() { 
        interrupted = true; 
        if (currentProcess) {
            currentProcess->kill();
            currentProcess->waitForFinished();
        }
        quit();
        wait();
    }

signals:
    void signalNCend();
    void signalError(QString s);
    void signalCanonLines(QVector<canonLine*> lines);
    void debugMessage(QString s);
    void gcodeLineMessage(QString s);
    void canonLineMessage(QString s);
    void signalCanonLine(canonLine* line);

protected:
    void run() override {
        exec();
    }

private:
    QString file;
    QString tooltable;
    QString interp;
    int gcode_lines = 0;
    int total_gcode_lines = 0;
    std::vector<canonLine*> lineVector;
    volatile bool interrupted = false;
    QProcess *currentProcess = nullptr;

    Pose initialPos;
    Pose userOrigin;

    void interpret_file_async();
    bool chooseToolTable();
    bool startInterp2(QProcess &tc, QString tempFile);
    void interpret2(QString tempFile);
    bool processCanonLine(std::string l);
    void infoMsg(std::string s);
};

}

#endif
