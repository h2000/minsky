/*
  @copyright Steve Keen 2012
  @author Russell Standish
  This file is part of Minsky.

  Minsky is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Minsky is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Minsky.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef PORT_H
#define PORT_H


struct Port
{
  bool input; //true if input port
  float x,y;
  int refCnt;
  Port(float x=0, float y=0, bool input=false): x(x), y(y), input(input), 
                                                refCnt(1) {}
  /// move port by a relative distance
  void move(float dx, float dy) {x+=dx; y+=dy;}
};


#include "port.cd"
#endif
