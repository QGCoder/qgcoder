/***************************************************************************
 *   Copyright (C) 2010 by Mark Pictor                                     *
 *   mpictor@gmail.com                                                     *
 *   modified by Kazuyasu Hamada 2015, k-hamada@gifu-u.ac.jp               *
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
/// Arc and helix moves.

#ifndef HELICALMOTION_HH
#define HELICALMOTION_HH

#include <string>
#include <vector>
#include <cmath>
#include <limits.h>

#include "point.hpp"
#include "canonMotion.hpp"
#include "canonLine.hpp"
#include "machineStatus.hpp"

namespace g2m {

/**
\class helicalMotion
\brief For the canonical command ARC_FEED.
This class handles both planar arcs and helical arcs. Inherits from canonMotion.
*/

class helicalMotion: protected canonMotion {
  /// the factory is the only thing that builds one of these
  friend canonLine* canonLine::canonLineFactory(std::string l, machineStatus s);

  public:
    /// create helical motion
    helicalMotion(std::string canonL, machineStatus prevStatus);
    MOTION_TYPE getMotionType() {return HELICAL;};
    /// return interpolated point along helix, a distance s from the start
    Point point(double s);
#ifdef MULTI_AXIS
    Point angle(double s);
#endif
    /// return the length of this helix move
    double length();     

  private:    
    void rotate(double &x, double &y, double c, double s);
    
// DATA, this corresponds to the "tokens" on the ARC_FEED canon-line
    double x1;       ///< endpoint of this move, abscissa
    double y1;       ///< endpoint of this move, ordinate
    double z1;       ///< endpoint of this move, applicate
    double a;        ///< A axis endpoint
    double b;        ///< B axis endpoint
    double c;        ///< C axis endpoint
    double rot;      ///< how many full turns the move makes
    double cx;       ///< centre of the arc, abscissa
    double cy;       ///< centre of the arc, ordinate

// these are the parameters calculated from the canon-params x1,y1,z1,a,b,c,rot,cx,cy
    unsigned int X;  ///< index of the first axis of the active plane
    unsigned int Y;  ///< index of the second
    unsigned int Z;  ///< index of the axis the helix rises along
    double d[6]; ///< 6-axis delta for this move, for linear interpolation
    double o[6]; ///< origin, i.e. the start point of the move
    double dtheta; ///< angle swept by this move; negative is clockwise
    double tx;     ///< vector from the centre to the start point, abscissa
    double ty;     ///< and its ordinate
    double radius; ///< radius of the arc
};

} // end namespace 

#endif //HELICALMOTION_HH
