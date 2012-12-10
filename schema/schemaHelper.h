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

#ifndef SCHEMA_HELPER
#define SCHEMA_HELPER

#include "../operation.h"
#include "../variable.h"
#include "../variableValue.h"

template <class T>
ecolab::array<T> toArray(const std::vector<T>& v) 
{
  ecolab::array<T> a(v.size());
  for (size_t i=0; i<v.size(); ++i) a[i]=v[i];
  return a;
}

template <class T>
std::vector<T> toVector(const ecolab::array<T>& a) 
{
  std::vector<T> v(a.size());
  for (size_t i=0; i<v.size(); ++i) v[i]=a[i];
  return v;
}

namespace minsky
{
  /**
     A bridge pattern to allow schemas to update private members of
     various classes, whilst retaining desired
     encapsulation. SchemaHelper is priveleged to allow access to
     private parts of the class to be initialised, but should only be
     used by schema classes.
  */
  struct SchemaHelper
  {
    // sets and gets the actual base stored values of coordinates
    template <class T> static float x(const T& o) {return o.m_x;}
    template <class T> static float y(const T& o) {return o.m_y;}
    template <class T> static void setXY(T& o, float x, float y)
    {o.m_x=x; o.m_y=y;}

    static void setPrivates(VariableBase& op, int outPort, int inPort) {
      op.m_outPort=outPort;
      op.m_inPort=inPort;
    }
    static void setPrivates(IntOp& op, const string& description, int intVar)
    {
      op.m_description=description;
      op.intVar=intVar;
    }
    static void setPrivates
    (minsky::GodleyTable& g, const vector<vector<string> >& data, 
     const vector<GodleyTable::AssetClass>& assetClass)
    {
      g.data=data;
      g.m_assetClass=assetClass;
    }

    static void setPrivates
    (minsky::GroupIcon& g, const vector<int>& ops, const vector<int>& vars,
     const vector<int>& wires, const vector<int>& inVariables, 
     const vector<int>& outVariables)
    {
      g.m_operations=ops;
      g.m_variables=vars;
      g.m_wires=wires;
      g.inVariables=inVariables;
      g.outVariables=outVariables;
    }

    static void setPrivates(minsky::VariableManager& vm, 
               const std::set<string>& w, const std::map<int, int>& p)
    {
      vm.wiredVariables=w;
      vm.portToVariable=p;
    }
  };

}

#endif
