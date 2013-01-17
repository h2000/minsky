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

#ifndef INGROUPTEST_H
#define INGROUPTEST_H
#include "groupIcon.h"
#include <TCL_obj_base.h>
#include <map>

namespace minsky
{
  /// support for determining if a point (x,y) lies within a group,
  /// and if so, which one
  class InGroup
  {
  public:
    struct Cell
    {
      float x0, x1, y0, y1, area; ///< rectangle bounds
      int id;
      Cell(): id(-1), x0(0), x1(0),y0(0),y1(0),area(0) {}
      Cell(int id, const GroupIcon& g);
      bool operator<(const Cell& x) const {return area<x.area;}
      bool inRect(float x, float y) const {
        return x>=x0 && x<=x1 && y>=y0 && y<=y1;
      }
    };
    typedef std::vector<std::vector<std::set<Cell> > > Cells;
       
  private:

    float xmin, xmax, ymin, ymax, xBinSz, yBinSz;
    Cells cells;
    CLASSDESC_ACCESS(InGroup);
  public:
    /// initialise with a collection of GroupIcons
    /// \a exclude specifies a group id to exclude from the test
    void initGroupList(const std::map<int, GroupIcon>&, int exclude=-1);
    /// return group containing (x,y) - if more than one group, then
    /// the smallest group (by area) is returned. If no group is
    /// applicable, -1 is returned
    int ContainingGroup(float x, float y) const;
    int containingGroup(TCL_args args) const {return ContainingGroup(args[0], args[1]);}
  };
}

#include "inGroupTest.cd"
#endif
