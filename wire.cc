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

#include "wire.h"
#include "zoom.h"
#include "portManager.h"
using namespace minsky;
using namespace ecolab;

void Wire::zoom(float xOrigin, float yOrigin, float factor)
{
  if (visible)
    for (size_t i=0; i<coords.size(); ++i)
      minsky::zoom(coords[i], (i&1)? yOrigin: xOrigin, factor);
}

void Wire::move(float dx, float dy)
{
  coords[pcoord(coords.size()/2)*2]+=dx;
  coords[pcoord(coords.size()/2)*2+1]+=dy;
  assert(coords.size()>=4);
  portManager().movePortTo(from, coords[0], coords[1]);
  portManager().movePortTo(to, coords[coords.size()-2], coords[coords.size()-1]);
}
