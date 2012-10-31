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
#ifndef WIRE_H
#define WIRE_H

#include <arrays.h>
//using ecolab::array_ns::array;

namespace minsky
{
  struct Wire
  {
    /// ports this wire connects
    int from, to;
    // for use in sets, etc
    bool operator<(const Wire& x) const {
      return from < x.from || from==x.from && to<x.to;
    }
    bool visible; ///<whether wire is visible on Canvas 
    /// display coordinates
    ecolab::array<float> coords;
    Wire(int from=0, int to=0, 
         const ecolab::array<float>& coords=ecolab::array<float>(), 
         bool visible=true): 
      from(from), to(to), coords(coords), visible(visible) {}
  };
}
#include "wire.cd"
#endif
