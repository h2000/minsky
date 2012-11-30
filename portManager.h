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
#ifndef PORTMANAGER_H
#define PORTMANAGER_H
#include "port.h"
#include "wire.h"
#include <vector>
#include <map>

#include <TCL_obj_base.h>
#include <assert.h>

namespace minsky
{
  class PortManager
  {
  public:

    typedef std::map<int, Port> Ports;
    typedef std::map<int, Wire> Wires;
    Ports ports;
    Wires wires;

    // add a port to the port map
    int addPort(const Port& p) {
      int nextId=ports.empty()? 0: ports.rbegin()->first+1;
      ports.insert(Ports::value_type(nextId, p));
      return nextId;
    }

    int addWire(Wire w); 
  
    void delPort(int port);
    /// move port to an absolute location
    void movePortTo(int port, float x, float y);
    /// move port by an increment
    void movePort(int port, float dx, float dy) {
      if (ports.count(port)) movePortTo(port, ports[port].x()+dx, ports[port].y()+dy);
    } 

    int ClosestPort(float x, float y);
    int ClosestOutPort(float x, float y);
    int ClosestInPort(float x, float y);

    /// return ID of the closest port
    int closestPort(ecolab::TCL_args args) 
    {return ClosestPort(args[0], args[1]);}
    /// return ID of the closest output port
    int closestOutPort(ecolab::TCL_args args) 
    {return ClosestOutPort(args[0], args[1]);}
    /// return ID of the closest output port
    int closestInPort(ecolab::TCL_args args) 
    {return ClosestInPort(args[0], args[1]);}

    void deleteWire(int id) {wires.erase(id);}
    ecolab::array<int> WiresAttachedToPort(int) const;

    /// return a list of wires attached to a \a port
    ecolab::array<int> wiresAttachedToPort(ecolab::TCL_args args) const
    {return WiresAttachedToPort(args);}

    ecolab::array<int> visibleWires() const;

  private:
    // the common closestPort implementation code
    template <class C> int closestPortImpl(float, float);

  };


  /// global portmanager
  PortManager& portManager();
  /// overrride default portManager
  void setPortManager(PortManager&);
}

#include "portManager.cd"
#endif
