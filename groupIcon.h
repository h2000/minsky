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
#ifndef GROUPICON_H
#define GROUPICON_H

#include <vector>
#include <algorithm>
#include <string>

#include "classdesc_access.h"
#include <TCL_obj_base.h>
#include <arrays.h>
using ecolab::array_ns::array;
using namespace ecolab;

class GroupIcon
{
public:
private:
  CLASSDESC_ACCESS(GroupIcon);
  std::vector<int> operations;
  std::vector<int> variables;
  std::vector<int> wires;
  array<int> m_ports;

public:
  std::string name;
  float x,y; //icon position
  float width, height; // size of icon
  float rotation; // orientation of icon
  const array<int>& ports() {return m_ports;}
  int numPorts() {return m_ports.size();}

  GroupIcon(): width(100), height(100), rotation(0) {}

  /// group all icons in rectangle bounded by (x0,y0):(x1,y1)
  void group(float x0, float y0, float x1, float y1);
  /// ungroup all icons
  void ungroup();

  /// populates this with a copy of src (with all internal objects
  /// registered with minsky)
  void copy(const GroupIcon& src);

  /// update port locations to current geometry and rotation.  Return
  /// the relative locations of the ports in unrotated coordinates as
  /// a vector of (x,y) pairs
  array<float> updatePortLocation();

  void MoveTo(float x1, float y1); ///< absolute move
  void moveTo(TCL_args args) {MoveTo(args[0], args[1]);}

};

#include "groupIcon.cd"
#endif
