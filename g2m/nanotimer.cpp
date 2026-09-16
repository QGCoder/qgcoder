/**************************************************************************
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
**************************************************************************/

#include "nanotimer.hpp"

// This used to call clock_gettime(CLOCK_MONOTONIC_RAW), which does not exist
// outside POSIX. std::chrono::steady_clock is the same monotonic clock and
// compiles everywhere.

namespace g2m {

void nanotimer::start() {
  begin = std::chrono::steady_clock::now();
}

long long nanotimer::getElapsed(){
  const auto delta = std::chrono::steady_clock::now() - begin;
  return std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
}

double nanotimer::getElapsedS(){
  const auto delta = std::chrono::steady_clock::now() - begin;
  return std::chrono::duration<double>(delta).count();
}

QString nanotimer::humanreadable(double s) {
  QString out;
  if (s > 60) {
    int m;
    m = s/60;
    s = s-(double)(m*60);
    out = QString::number(m)  + QString("m, ");
  }
  if (s > .5) {
    out += QString::number(s); 
    out += QString(" s");
  } else if (s> 0.0005) {
    out = QString::number(s*1000); 
    out += QString(" ms");
  } else {
    out = QString::number(s*1000000); 
    out += QString(" us");
  }
  return out;
}


} // end namespace
