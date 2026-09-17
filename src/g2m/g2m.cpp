/***************************************************************************
*   Copyright (C) 2010 by Mark Pictor                                      *
*   mpictor@gmail.com                                                      *
*   modified by Anders Wallin 2011, anders.e.e.wallin@gmail.com            *
*   modified by Kazuyasu Hamada 2015, k-hamada@gifu-u.ac.jp                *
*                                                                          *
*   This program is free software; you can redistribute it and/or modify   *
*   it under the terms of the GNU General Public License as published by   *
*   the Free Software Foundation; either version 2 of the License, or      *
*   (at your option) any later version.                                    *
*                                                                          *
*   This program is distributed in the hope that it will be useful,        *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
*   GNU General Public License for more details.                           *
*                                                                          *
*   You should have received a copy of the GNU General Public License      *
*   along with this program; if not, write to the                          *
*   Free Software Foundation, Inc.,                                        *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.              *
***************************************************************************/

/// \file
/// \see g2m::g2m

#include <iostream>
#include <cmath>
#include <fstream>
#include <stdlib.h>

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include "g2m.hpp"
#include "nanotimer.hpp"
#include "machineStatus.hpp"
#include "interp_driver.hpp"

namespace g2m {

/// Checks the file is one we can interpret, then runs it.
/// \see interpret_file_async()
void g2m::interpret_file() {
    if (file.isEmpty() || (!file.endsWith(".ngc") && !file.endsWith(".canon"))) {
        infoMsg("No valid g-code file to interpret");
        return;
    }
    
    interpret_file_async();
}

/// Prepares the file and runs it through the interpreter, turning the
/// canonical output into canonLine objects as it arrives.
void g2m::interpret_file_async() {
    nanotimer timer;
    timer.start();
    gcode_lines = 0;

    if ( file.endsWith(".ngc") ) {
            // push g-code lines to ui:
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
            // file opened successfully
            QTextStream t( &fileHandle );        // use a text stream
            QTextStream out( &tempFileHandle );
            // until end of file...
            while ( !t.atEnd() ) {           
                // read and parse the command line
                gline = t.readLine();         // line of text excluding '\n'
                glinebuffer += gline + '\n';
                out << "(Gcode Line No." << gcode_lines << ")\n";
                out << gline + '\n';
                gcode_lines++;
            }
            fileHandle.close();
            tempFileHandle.close();
        }

        while (glinebuffer.right(1) == "\n")
        	glinebuffer.remove(glinebuffer.length() - 1, 1); // remove last '\n' s

        emit gcodeLineMessage(glinebuffer);
        
        emit debugMessage( tr("g2m: interpreting  %1").arg(file) ); 
        interpret(tempFileHandle.fileName());
    } else if (file.endsWith(".canon")) { //just process each line
        std::ifstream inFile(file.toLatin1());
        std::string sLine;
        QString sLinebuffer;

        while(std::getline(inFile, sLine)) {
            if (sLine.length() > 1) {  //helps to prevent segfault in canonLine::cmdMatch()
            	sLinebuffer += sLine.c_str() + QString("\n");
                processCanonLine(sLine); // requires no interpret()
            }
        }
        emit canonLineMessage(sLinebuffer);
    } else {
        emit debugMessage( tr("File name must end with .ngc or .canon!") ); 
        return;
    }
    
    double e = timer.getElapsedS();
    emit debugMessage( tr("g2m: Total time to process that file: ") +  timer.humanreadable(e)  ) ;
    //std::cout << "Total time to process that file: " << timer.humanreadable(e).toStdString() << std::endl;
    lineVector.clear();
}

/// The interpreter falls back to its own built-in tool table, so an unset or
/// missing tool table is not an error - it just means "use the default".
/// \returns the tool table to hand the interpreter, or an empty string to
///          let it fall back on its built-in default. A path that does not
///          exist is reported and treated as empty rather than failing.
QString g2m::toolTablePath() {
  if (tooltable.isEmpty())
    return QString();
  if (!QFileInfo(tooltable).exists()) {
    emit debugMessage( tr("g2m: tool table %1 not found, using the built-in default").arg(tooltable) );
    return QString();
  }
  return tooltable;
}

/// process a canon-line input string. this is a canon-string from rs274.
/// call canonLineFactory to produce a canonLine and save it to lineVector
/// \param l the canon line, newline terminated
/// \returns true once the end of the program has been seen
bool g2m::processCanonLine(std::string l) {
    canonLine* cl;
    if (lineVector.size() == 0) {
        // no status exists, so make one up.
        cl = canonLine::canonLineFactory(l, machineStatus( initialPos, userOrigin ));
    } else {
        // use the last element status
        cl = canonLine::canonLineFactory(l, *(lineVector.back())->getStatus());
    }
    emit signalCanonLine(cl);
    lineVector.push_back(cl); 

    //if ( debug ) 
        // std::cout << "Line " << cl->getLineNum() << "/N" << cl->getN() <<  std::endl;

    // return true when we reach end-of-program
    if (!cl->isMotion()) {
        if (cl->isNCend()) {
            emit signalNCend();
        }
        return cl->isNCend();
    }

    return false;
}

/// output information to std::cout
/// \param s the message
void g2m::infoMsg(std::string s) {
    std::cout << s << std::endl;
}

/// Run the embedded rs274ngc interpreter over tempFile and turn every
/// canonical command it produces into a canonLine.
/// \param tempFile the prepared file to interpret
void g2m::interpret(QString tempFile) {
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
        });

    emit canonLineMessage( l.left(l.size()-1) );

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
        infoMsg("Warning: file data not terminated correctly. If the file is terminated correctly, this indicates a problem interpreting the file.");
        emit debugMessage("Warning: file data not terminated correctly. If the file is terminated correctly, this indicates a problem interpreting the file.");
    }

    emit debugMessage( tr("g2m: read %1 lines of g-code which produced %2 canon-lines.").arg(gcode_lines).arg(lineVector.size()) );
}

} // end namespace
