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
#include "variable.h"
#include "ecolab_epilogue.h"
using namespace classdesc;

void VariableBase::addPorts()
{
  if (numPorts()>0)
    m_outPort = portManager().addPort(Port(x,y,false));
  if (numPorts()>1)
    m_inPort = portManager().addPort(Port(x,y,true));
}

void VariableBase::delPorts()
{
  if (m_outPort>=0) portManager().delPort(m_outPort);
  if (m_inPort>=0) portManager().delPort(m_inPort);
  m_inPort=m_outPort=-1;
}

void VariableBase::toggleInPort()
{
  if (type()==integral)
    if (m_inPort==-1)
      m_inPort = portManager().addPort(Port(x,y,true));
    else 
      {
        portManager().delPort(m_inPort);
        m_inPort=-1;
      }
}

////Factory
VariableBase* VariableBase::create(VariableType::Type type)
{
  switch (type)
    {
    case undefined: return new Variable<undefined>; break;
    case flow: return new Variable<flow>; break;
    case stock: return new Variable<stock>; break;
    case tempFlow: return new Variable<tempFlow>; break;
    case integral: return new Variable<integral>; break;
    default: assert( false && "invalid variable type");
      return NULL;
    }
}

template <> int Variable<VariableBase::undefined>::numPorts() const {return 0;}
template <> int Variable<VariableBase::flow>::numPorts() const {return 2;}
template <> int Variable<VariableBase::stock>::numPorts() const {return 1;}
template <> int Variable<VariableBase::tempFlow>::numPorts() const {return 2;}
template <> int Variable<VariableBase::integral>::numPorts() const 
{
  return inPort()<0? 1: 2;
}

void VariableBase::MoveTo(float x1, float y1) 
{
  float dx=x1-x, dy=y1-y;
  move(dx, dy);
  x=x1; y=y1;
}

void VariableBase::move(float dx, float dy) 
{
  x+=dx; y+=dy;
  if (m_outPort!=-1)
    portManager().movePort(m_outPort, dx, dy);
  if (m_inPort!=-1)
    portManager().movePort(m_inPort, dx, dy);
    
}



array<int> VariableBase::ports() const 
{
  array<int> r(numPorts());
  r[0]=outPort();
  if (numPorts()>1) r[1]=inPort();
  return r;
}
