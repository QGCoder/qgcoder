/***************************************************************************
 *   Copyright (C) 2010 by Mark Pictor                                     *
 *   mpictor@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/// \file
/// Canon lines that change state without moving the tool.

#ifndef CANONMOTIONLESS_HH
#define CANONMOTIONLESS_HH

#include <string>
#include <vector>

#include <cmath>
#include <limits.h>

#include "canonLine.hpp"
#include "machineStatus.hpp"

namespace g2m {

/**
\class canonMotionless
\brief A canonical command that (generally) does not cause or alter axis motion
This class is for anything other than STRAIGHT_FEED, STRAIGHT_TRAVERSE, and ARC_FEED - 
* including changes in feedrate, spindle speed, tool, coolant, ending the program, etc
*/

class canonMotionless: protected canonLine {
  /// the factory is the only thing that builds one of these
  friend canonLine* canonLine::canonLineFactory(std::string l, machineStatus s);
  public:
    /// create motionless canon-line
    /// \param canonL the canon line \param prevStatus state to start from
    canonMotionless(std::string canonL, machineStatus prevStatus);
    /// \returns false, always: this command does not move the tool
    bool isMotion() {return false;};
    /// \returns MOTIONLESS, always
    MOTION_TYPE getMotionType() {return MOTIONLESS;};
    /// \returns true when this command validly terminates the program
    bool isNCend() {return ncEnd;};
  protected:
    /// flag(?)
    bool match;
    /// flag indicating that this canon-line is handled
    bool handled;
    /// flag indicating end of g-code program
    bool ncEnd;
};

} // end namespace

#endif //CANONMOTIONLESS_HH
