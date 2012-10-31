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
#ifndef GODLEYICON_H
#define GODLEYICON_H

#include "variable.h"
#include "godley.h"
#include <map>

namespace minsky
{
  class Minsky;

  struct GodleyIcon
  {
    float adjustHoriz, adjustVert; // difference between where variables are displayed and screen coordinates
    float x, y; ///< position of Godley icon
    float scale; ///< scale factor of the XGL image
    typedef std::vector<VariablePtr> Variables;
    Variables flowVars, stockVars;
    GodleyTable table;
    /// updates the variable lists with the Godley table
    void update();

    int numPorts() const;
    array<int> ports() const;
    void MoveTo(float x1, float x2);
    void moveTo(TCL_args args) {MoveTo(args[0], args[1]);}

    /// returns the name of a variable if point (x,y) is within a
    /// variable icon, "@" otherwise, indicating that the Godley table
    /// has been selected.
    int Select(float x, float y);
    int select(TCL_args args) {return Select(args[0],args[1]);}

    /// override GodleyIcon's minsky reference
    static void setMinsky(Minsky&);

  };

}

#include "godleyIcon.cd"
#endif
