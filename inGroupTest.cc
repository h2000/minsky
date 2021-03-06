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

#include "inGroupTest.h"

using namespace minsky;
using namespace std;

InGroup::Cell::Cell(int id, const GroupIcon& g): 
  id(id), 
  y0(g.y()-0.5*g.height*g.zoomFactor), 
  y1(g.y()+0.5*g.height*g.zoomFactor), 
  area(g.width*g.height*g.zoomFactor* g.zoomFactor) 
{
  float left=0, right=0;
  //g.margins(left, right);
  x0=g.x()-(0.5*g.width-left)*g.zoomFactor;
  x1=g.x()+(0.5*g.width-right)*g.zoomFactor;
}


namespace
{
  void excludeSelfAndChildren(set<int>& excludeIds, 
                              const map<int, GroupIcon>& g, int id)
  {
    excludeIds.insert(id);
    map<int, GroupIcon>::const_iterator excludeGroup=g.find(id);
    if (excludeGroup!=g.end())
      {
        const vector<int>& children=excludeGroup->second.groups();
        for (vector<int>::const_iterator i=children.begin(); 
             i!=children.end(); ++i)
          excludeSelfAndChildren(excludeIds, g, *i);
      }
  }
     
}

void InGroup::initGroupList(const map<int, GroupIcon>& g, int exclude)
{
  cells.clear();
  // construct all the Cells
  vector<Cell> rects;
  set<int> excludeIds;
  excludeSelfAndChildren(excludeIds, g, exclude);

  for (std::map<int, GroupIcon>::const_iterator i=g.begin(); i!=g.end(); ++i)
    if (excludeIds.count(i->first)==0)
      rects.push_back(Cell(i->first, i->second));

  // compute total bounds
  ymin=xmin=numeric_limits<float>::max(); ymax=xmax=-xmin;
  for (vector<Cell>::const_iterator i=rects.begin(); i!=rects.end(); ++i)
    {
      if (xmin>i->x0) xmin=i->x0;
      if (xmax<i->x1) xmax=i->x1;
      if (ymin>i->y0) ymin=i->y0;
      if (ymax<i->y1) ymax=i->y1;
    }
  if (g.empty()) return;

  // start by assuming rectangles are even distributed, so divide
  // total area into g.size() cells. If all the rectangles are stacked
  // on top of each other, this algorithm will be unbalanced, but that
  // is fairly pathological, as well designed layouts will spread out
  // the groups
  float sqrtNoGroups=sqrt(g.size());
  xBinSz=(xmax-xmin)/sqrtNoGroups;
  yBinSz=(ymax-ymin)/sqrtNoGroups;
  cells.resize(int(sqrtNoGroups+1), 
               std::vector<std::set<Cell> >(int(sqrtNoGroups+1)));
  for (vector<Cell>::const_iterator i=rects.begin(); i!=rects.end(); ++i)
    for (float x=i->x0; x<(int((i->x1-xmin)/xBinSz)+1)*xBinSz+xmin; x+=xBinSz)
      for (float y=i->y0; y<(int((i->y1-ymin)/yBinSz)+1)*yBinSz+ymin; y+=yBinSz)
        cells[int((x-xmin)/xBinSz)][int((y-ymin)/yBinSz)].insert(*i);
}




int InGroup::ContainingGroup(float x, float y) const
{
  if (x<xmin||x>xmax||y<ymin||y>ymax) return -1;

  /// rectangles are binned into cells of size (xBinSz, yBinSz),
  /// lookup searches linearly over cell contents, sorted by rectangle
  /// size
  int xi=(x-xmin)/xBinSz, yi=(y-ymin)/yBinSz;
  for (set<Cell>::const_iterator i=cells[xi][yi].begin(); i!=cells[xi][yi].end(); ++i)
    if (i->inRect(x,y))
      return i->id;
  return -1;
}
