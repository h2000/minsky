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
#include "portManager.h"
#include "minsky.h"
#include <tcl++.h>
#include <ecolab_epilogue.h>

using namespace std;
using namespace minsky;

PortManager& minsky::portManager() {return minsky::minsky();}

int PortManager::addWire(Wire w) 
{
  // adjust end points to be aligned with ports
  array<float> coords=w.Coords();
  if (coords.size()<4)
    coords.resize(4);
  const Port& to=ports[w.to];
  const Port& from=ports[w.from];
  coords[0]=from.x();
  coords[1]=from.y();
  coords[coords.size()-2]=to.x();
  coords[coords.size()-1]=to.y();
  w.Coords(coords);

  int nextId=wires.empty()? 0: wires.rbegin()->first+1;
  wires.insert(Wires::value_type(nextId, w));
  
  assert(minsky().variables.noMultipleWiredInputs());
  return nextId;
}


void PortManager::movePortTo(int port, float x, float y)
{
  if (ports.count(port))
    {
      Port& p=ports[port];
      p.m_x=x; p.m_y=y;
      array<int> attachedWires=WiresAttachedToPort(port);
      for (array<int>::iterator i=attachedWires.begin(); 
           i!=attachedWires.end(); ++i)
        {
          assert(wires.count(*i));
          Wire& w=wires[*i];
          array<float> coords=w.Coords();
          assert(coords.size()>=4);
          if (w.from==port)
            {
              coords[0]=p.x();
              coords[1]=p.y();
            }
          else if (w.to==port)
            {
              coords[coords.size()-2]=p.x();
              coords[coords.size()-1]=p.y();
            }
          w.Coords(coords);
        }
    }
}

template <class C>
int PortManager::closestPortImpl(float x, float y)
{
  int port=-1;
  float minr=numeric_limits<float>::max();
  for (Ports::const_iterator i=ports.begin(); i!=ports.end(); ++i)
    if (C::eval(i->second))
    {
      const Port& p=i->second;
      float r=(x-p.x())*(x-p.x()) + (y-p.y())*(y-p.y());
      if (r<minr)
        {
          minr=r;
          port=i->first;
        }
    }
  return port;
}

namespace
{
  struct AlwaysTrue
  {
    inline static bool eval(const Port& p) {return true;}
  };
  struct InputTrue
  {
    inline static bool eval(const Port& p) {return p.input;}
  };
  struct OutputTrue
  {
    inline static bool eval(const Port& p) {return !p.input;}
  };
}


int PortManager::ClosestPort(float x, float y)
{return closestPortImpl<AlwaysTrue>(x,y);}

int PortManager::ClosestOutPort(float x, float y)
{return closestPortImpl<OutputTrue>(x,y);}

int PortManager::ClosestInPort(float x, float y)
{return closestPortImpl<InputTrue>(x,y);}

array<int> PortManager::WiresAttachedToPort(int port) const
{
  array<int> ret;
  for (Wires::const_iterator w=wires.begin(); w!=wires.end(); ++w)
    if (w->second.to == port || w->second.from == port)
      ret <<= w->first;
  return ret;
}

void PortManager::delPort(int port)
{
  if (port>=0)
    {
      array<int> wires=WiresAttachedToPort(port);
      for (size_t i=0; i<wires.size(); ++i)
        deleteWire(wires[i]);
      ports.erase(port);
    }
}


array<int> PortManager::visibleWires() const
{
  array<int> ret;
  for (Wires::const_iterator w=wires.begin(); w!=wires.end(); ++w)
    if (w->second.visible)
      ret<<=w->first;
  return ret;
}
