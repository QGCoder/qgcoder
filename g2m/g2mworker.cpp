#include "g2mworker.h"

#include <iostream>

#include "rs274ngc_interp.hpp"

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
        // Truncate: without it a shorter g-code file leaves the tail of the
        // previous one behind, and the interpreter reads that too.
        if ( !tempFileHandle.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
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
        interpret(tempFileHandle.fileName());
    } else if (file.endsWith(".canon")) {
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

/// The interpreter falls back to its own built-in tool table, so an unset or
/// missing tool table is not an error - it just means "use the default".
QString G2mWorker::toolTablePath() {
    if (tooltable.isEmpty())
        return QString();
    if (!QFileInfo(tooltable).exists()) {
        emit debugMessage( tr("g2m: tool table %1 not found, using the built-in default").arg(tooltable) );
        return QString();
    }
    return tooltable;
}

/// Run the embedded rs274ngc interpreter over tempFile and turn every
/// canonical command it produces into a canonLine.
void G2mWorker::interpret(QString tempFile) {
    rs274ngc::Interpreter interp;
    interp.setToolTable(toolTablePath().toStdString());

    QString l;
    bool foundEOF = false;

    rs274ngc::Result result = interp.interpretFile(
        tempFile.toStdString(),
        [&](const std::string &canon) {
            if (canon.find("COMMENT(\"Gcode Line No.") != std::string::npos) {
                total_gcode_lines++;
                return;
            }
            // canonLine's tokenizer counts the trailing newline as a token, so
            // keep the line terminated exactly as it was when it arrived over
            // a pipe from the interpreter process.
            std::string line = canon + "\n";
            l += QString::fromStdString(line);
            if (processCanonLine(line))
                foundEOF = true;
        },
        [this]() { return interrupted; });

    if (result.aborted)
        return;

    if (!l.isEmpty())
        emit canonLineMessage(l);

    QVector<canonLine*> allLines;
    for (canonLine* line : lineVector) {
        allLines.append(line);
    }
    emit signalCanonLines(allLines);

    if (!result.ok) {
        infoMsg("Interpreter exited with error:\n" + result.error);
        emit debugMessage( tr("Interpreter exited with error:\n%1").arg(QString::fromStdString(result.error)) );
        emit signalError( tr("Interpreter exited with error:\n%1").arg(QString::fromStdString(result.error)) );
        return;
    }

    if (!foundEOF) {
        emit debugMessage("Note: G-code file processed (no explicit M2/M30 end marker found)");
    }

    emit debugMessage( tr("g2m: read %1 lines of g-code which produced %2 canon-lines.").arg(gcode_lines).arg(lineVector.size()) );
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
