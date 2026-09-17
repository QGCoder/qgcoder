/// \file
/// The interpreter run, moved off the GUI thread.

#ifndef G2MWORKER_H
#define G2MWORKER_H

#include <QThread>
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

/// \brief Runs the g-code interpreter away from the GUI thread.
///
/// The object is moved onto its own thread and driven entirely by queued
/// signals, so a long interpretation never blocks the window. Canon lines are
/// emitted one at a time as they are produced and again as a whole vector at
/// the end; stop() asks the run in progress to give up at the next line.
class G2mWorker : public QThread
{
    Q_OBJECT

public:
    /// \param parent owner, as usual for QObject
    G2mWorker(QObject *parent = nullptr) : QThread(parent) {
        QObject::setObjectName("G2mWorker");
    }

    /// \param f the .ngc or .canon file to interpret on the next process()
    void setFile(QString f) { file = f; }
    /// \param tbl tool table path; empty means the interpreter's own default
    void setToolTable(QString tbl) { tooltable = tbl; }

public slots:
    /// Asks the run in progress to give up at the next canon line.
    void stop() { interrupted = true; }
    /// Interprets the file set by setFile(). Connect queued: this runs for as
    /// long as the file takes.
    void process() {
        qDebug() << "G2mWorker::process() called";
        interpret_file_async();
    }
    /// Stops the run, ends the thread's event loop and waits for it.
    void quitAndDelete() {
        interrupted = true;
        quit();
        wait();
    }

signals:
    /// the program's end marker (M2 / M30) has been reached
    void signalNCend();
    /// the interpreter stopped early; \param s the message to show
    void signalError(QString s);
    /// the finished tool path \param lines every canon line of the run
    void signalCanonLines(QVector<canonLine*> lines);
    /// progress and timing for the debug pane \param s the message
    void debugMessage(QString s);
    /// one line of g-code as it is read \param s the line
    void gcodeLineMessage(QString s);
    /// the canon output as text \param s the accumulated lines
    void canonLineMessage(QString s);
    /// one canon line, as soon as it is produced \param line the new line
    void signalCanonLine(canonLine* line);

protected:
    /// Runs an event loop, so queued calls to process() arrive here.
    void run() override {
        exec();
    }

private:
    QString file;                        ///< file to interpret
    QString tooltable;                   ///< tool table, empty for the default
    int gcode_lines = 0;                 ///< lines of g-code read
    int total_gcode_lines = 0;           ///< line-number comments seen
    std::vector<canonLine*> lineVector;  ///< canon lines of the current run
    volatile bool interrupted = false;   ///< set by stop(), read by the run

    Pose initialPos;                     ///< where the machine starts
    Pose userOrigin;                     ///< active origin offset

    /// interpret file, from validation through to the finished signals
    void interpret_file_async();
    /// tool table to hand the interpreter; empty means use its built-in default
    QString toolTablePath();
    /// run one prepared file through the interpreter
    void interpret(QString tempFile);
    /// turn one canon line into a canonLine; true at the end of the program
    bool processCanonLine(std::string l);
    /// report a message on stdout
    void infoMsg(std::string s);
};

}

#endif
