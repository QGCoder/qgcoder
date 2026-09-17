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
/// The machine state carried from one canon line to the next.

#ifndef MACHINESTATUS_HH
#define MACHINESTATUS_HH

#include <map>
#include <limits.h> //to fix "error: INT_MIN was not declared in this scope"
#include "point.hpp"

namespace g2m {

/// the current plane, XY, YZ, or XZ
enum CANON_PLANE { CANON_PLANE_XY, CANON_PLANE_YZ, CANON_PLANE_XZ };
/// the status of the spindle
enum SPINDLE_STATUS { OFF = 0x100, CW = 0x200, CCW = 0x400, BRAKE = 0x800 };
/// motion type
enum MOTION_TYPE { NOT_DEFINED = 0x0, MOTIONLESS = 0x1, HELICAL = 0x2, STRAIGHT_FEED = 0x4, TRAVERSE = 0x8 };


/// coolant-status
struct coolantStruct {
    /// is flood on?
    bool flood; 
    /// is mist on?
    bool mist; 
    /// is the spindle on?
    bool spindle;
};

/**
\class machineStatus
\brief This class contains the machine's state for one canonical command.
The information stored includes the 
* coolant state, spindle speed and direction, feedrate, start and end pose, tool in use

Important: 'pose' refers to how the machine's axes are positioned, 
* while 'direction' refers to the direction of motion
*/
class machineStatus {
  public:
    /// Chains on from the previous move: this move's start is that move's end.
    /// \param oldStatus the state the previous move left behind
    machineStatus(machineStatus const& oldStatus);
    /// start of the program, from the interpreter's variable file
    machineStatus(Pose initial);
    /// start of the program, with an origin offset already in force
    machineStatus(Pose initial, Pose userOrigin);
    /// \param m what kind of move this is
    void setMotionType(MOTION_TYPE m);
    /// \param newPose where this move ends up
    void setEndPose(Pose newPose);
    /// set endPose
    /// \param p where this move ends up, direction unchanged
    void setEndPose(Point p);
    /// set current feedrate \param f the new feed rate
    void setFeed(const double f) { F = f; };
    /// set spindle speed \param s the new speed
    void setSpindleSpeed(const double s) { S = s; };
    /// set spindle status \param s the new status
    void setSpindleStatus(const SPINDLE_STATUS s) { spindleStat = s; };
    /// set coolant status \param c the new coolant state
    void setCoolant(coolantStruct c) { coolant = c; };
    /// set the current plane \param p the new plane
    void setPlane(CANON_PLANE p) { plane = p; };
    /// set the current origin \param newOrigin the new origin offset
    void setOrigin(Pose newOrigin) { origin = newOrigin; };
    /// \returns the current feedrate
    double getFeed() const { return F; };
    /// \returns the spindle speed
    double getSpindleSpeed() const { return S; };
    /// \returns the spindle status
    SPINDLE_STATUS getSpindleStatus() const { return spindleStat; };
    /// \returns the coolant state
    const coolantStruct getCoolant() { return coolant; };
    /// \returns the machine pose this move starts from
    const Pose getStartPose() { return startPose; };
    /// \returns the machine pose this move ends at
    const Pose getEndPose() { return endPose; };
    /// \returns the current plane
    CANON_PLANE getPlane() const { return plane; };
    /// \returns the current origin offset
    Pose getOrigin() const { return origin; };
    /// set startDir \param d direction this move sets off in
    void setStartDir( Point d) { startDir = d; };
    /// \returns the direction this move sets off in
    const Point getStartDir() const { return startDir; };
    /// set endDir \param d direction this move arrives on
    void setEndDir( Point d) { endDir = d; };
    /// \returns the direction this move arrives on
    const Point getEndDir() const { return endDir; };
    /// \returns the direction the previous move arrived on
    const Point getPrevEndDir() const { return prevEndDir; };
    /// Resets every field to its default, as at the start of a program.
    void clearAll(void);
    /// \returns true while this is the first move of the program
    bool isFirst() { return first; };
    /// set the tool index \param n ID of the tool to be used
    void setTool(int n); //n is the ID of the tool to be used.
    /// \returns the current tool index
    int  getTool() const { return myTool; };
    /// \returns spindle status and motion type, or-ed together
    int  getSpindleMotionStatus() const { return static_cast<int>(spindleStat) | static_cast<int>(motionType); }

  protected:
    /// machine Pose at start of this move
    Pose startPose;
    /// machine Pose at end of this move
    Pose endPose;
    /// feed-rate
    double F;
    /// spindle speed
    double S;  
    /// coolant status
    coolantStruct coolant;
    /// the direction in which the current move starts
    Point startDir;
    /// the direction in which the current move ends
    Point endDir;
    /// endDir of the previous move
    Point prevEndDir;
    /// flag indicating very first move of g-code program(?)
    bool first;
    /// misc flag(?)
    bool lastMotionWasTraverse;
    /// index of current tool
    int myTool;
    /// the current motion type
    MOTION_TYPE motionType;
    /// the status of the spindle
    SPINDLE_STATUS spindleStat;
    /// the current plane (used e.g. for G2 and G3 moves), either XY, XZ, or YZ
    CANON_PLANE plane;
    /// the current origin
    Pose origin;

  private:
    machineStatus();  //prevent use of this ctor by making it private 
};

} // end namespace

#endif //MACHINESTATUS_HH
