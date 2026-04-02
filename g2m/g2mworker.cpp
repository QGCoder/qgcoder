#include "g2mworker.h"

#include <iostream>

namespace g2m {

void G2mWorker::interpret_file_async() {
    interrupted = false;
    nanotimer timer;
    timer.start();
    gcode_lines = 0;

    if ( file.isEmpty() || (!file.endsWith(".ngc") && !file.endsWith(".canon"))) {
        infoMsg("No valid g-code file to interpret");
        return;
    }

    if ( file.endsWith(".ngc") ) {
        QFile fileHandle( file );
        QString gline;
        QString glinebuffer;
        QString tempFile = "/tmp/cutsim.temp";
        QFile	tempFileHandle( tempFile );
        if ( !tempFileHandle.open(QIODevice::ReadWrite | QIODevice::Text))
        	return;

        if ( fileHandle.open( QIODevice::ReadOnly | QIODevice::Text) ) {
            QTextStream t( &fileHandle );
            QTextStream out( &tempFileHandle );
            while ( !t.atEnd() ) {
                gline = t.readLine();
                glinebuffer += gline + '\n';
                out << "(Gcode Line No." << gcode_lines << ")\n";
                out << gline + '\n';
                gcode_lines++;
            }
            fileHandle.close();
            tempFileHandle.close();
        }

        while (glinebuffer.right(1) == "\n")
        	glinebuffer.remove(glinebuffer.length() - 1, 1);

        emit gcodeLineMessage(glinebuffer);
        emit debugMessage( tr("g2m: interpreting  %1").arg(file) );
        interpret2(tempFileHandle.fileName());
    } else if (file.endsWith(".canon")) {
        if (!chooseToolTable()) {
            infoMsg("Can't find tool table. Aborting.");
            emit debugMessage("Can't find tool table. Aborting.");
            return;
        }

        std::ifstream inFile(file.toLatin1());
        std::string sLine;
        QString sLinebuffer;

        while(std::getline(inFile, sLine)) {
            if (sLine.length() > 1) {
            	sLinebuffer += sLine.c_str() + QString("\n");
                processCanonLine(sLine);
            }
        }
        emit canonLineMessage(sLinebuffer);
    } else {
        emit debugMessage( tr("File name must end with .ngc or .canon!") );
        return;
    }

    double e = timer.getElapsedS();
    emit debugMessage( tr("g2m: Total time to process that file: ") +  timer.humanreadable(e)  ) ;
    lineVector.clear();
}

bool G2mWorker::chooseToolTable() {
  if (tooltable.isEmpty() || !QFileInfo(tooltable).exists()){
    QString defaultTooltable = QDir::tempPath() + "/qgcoder.tooltable";
    QFile file(defaultTooltable);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "T1 P1 Z0.0 D0.125000 ; 1/8 inch end mill\n";
        out << "T2 P2 Z0.0 D0.062500 ; 1/4 inch end mill\n";
        file.close();
        tooltable = defaultTooltable;
        return true;
    }
    infoMsg(" cannot find tooltable! ");
    emit debugMessage(" cannot find tooltable! ");
    return false;
  }
  return true;
}

bool G2mWorker::startInterp2(QProcess &tc, QString tempFile) {
    if (!chooseToolTable())
        return false;
    tc.setProcessChannelMode(QProcess::SeparateChannels);
    tc.start( interp , QStringList(tempFile) );

    if (!tc.waitForStarted(5000)) {
        infoMsg("Interpreter failed to start");
        return false;
    }

    tc.write("3\n");

    QString resp;

    tc.waitForReadyRead(100);
    resp = tc.readAllStandardOutput();

    QByteArray toolPath = tooltable.toLocal8Bit();
    tc.write(toolPath);
    tc.write("\n");

    if (!tc.waitForReadyRead(3000)) {
    }
    resp = tc.readAllStandardOutput();

    if (resp.contains("Cannot open")) {
        infoMsg("Error: Cannot open tooltable file. Check file permissions and path.");
        emit debugMessage("Cannot open tooltable: " + tooltable);
        return false;
    }

    if (tc.state() == QProcess::NotRunning) {
        return false;
    }

    tc.write("1\n");
    tc.closeWriteChannel();
    return true;
}

void G2mWorker::interpret2(QString tempFile) {
    QProcess toCanon;
    currentProcess = &toCanon;
    bool foundEOF = false;

    if (!startInterp2(toCanon, tempFile)) {
        currentProcess = nullptr;
        return;
    }

    if (toCanon.state() == QProcess::NotRunning) {
        infoMsg("Interpreter died. Bad tool table?");
        emit debugMessage("Interpreter died. Bad tool table?");
        toCanon.close();
        currentProcess = nullptr;
        return;
    }

    qint64 lineLength;
    char line[260];
    QString l;
    QString cmt;

    do {
        if (interrupted) {
            toCanon.kill();
            toCanon.waitForFinished();
            currentProcess = nullptr;
            return;
        }
        if (toCanon.waitForReadyRead(100)) {
            while (toCanon.canReadLine()) {
                if (interrupted) {
                    toCanon.kill();
                    toCanon.waitForFinished();
                    currentProcess = nullptr;
                    return;
                }
                lineLength = toCanon.readLine(line, sizeof(line));
                if (lineLength != -1) {
                    cmt = line;
                    if (cmt.contains("COMMENT(\"Gcode Line No."))
                        total_gcode_lines++;
                    else {
                        l += line;
                        foundEOF = processCanonLine(line);
                    }
                }
            }
        }
    } while (toCanon.state() != QProcess::NotRunning);

    currentProcess = nullptr;

    if (!l.isEmpty())
        emit canonLineMessage(l);

    if (!foundEOF) {
        QVector<canonLine*> allLines;
        for (canonLine* line : lineVector) {
            allLines.append(line);
        }
        emit signalCanonLines(allLines);
    }

  std::string s = (const char *)toCanon.readAllStandardError();
  s.erase(0,s.find("executing"));
  if (s.size() > 10) {
    infoMsg("Interpreter exited with error:\n"+s.substr(10));
    emit debugMessage(("Interpreter exited with error:\n"+s.substr(10)).c_str());
    return;
  }

  if (!foundEOF) {
    emit debugMessage("Note: G-code file processed (no explicit M2/M30 end marker found)");
  }

    emit debugMessage( tr("g2m: read %1 lines of g-code which produced %2 canon-lines.").arg(gcode_lines).arg(lineVector.size()) );
    return;
}

bool G2mWorker::processCanonLine(std::string l) {
    canonLine* cl;
    if (lineVector.size() == 0) {
        cl = canonLine::canonLineFactory(l, machineStatus( initialPos, userOrigin ));
    } else {
        cl = canonLine::canonLineFactory(l, *(lineVector.back())->getStatus());
    }
    emit signalCanonLine(cl);
    lineVector.push_back(cl);

    if (!cl->isMotion()) {
        if (cl->isNCend()) {
            emit signalNCend();
        }
        return cl->isNCend();
    }

    return false;
}

void G2mWorker::infoMsg(std::string s) {
    std::cout << s << std::endl;
}

}
