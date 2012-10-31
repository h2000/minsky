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
    static void setPrivates(VariableBase& op, int outPort, int inPort) {
      op.m_outPort=outPort;
      op.m_inPort=inPort;
    }
    static void setPrivates(Operation& op, const std::vector<int>& ports, 
                            const string& description, int intVar)
    {
      op.m_ports=ports;
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
     const vector<int>& wires, const ecolab::array<int>& ports)
    {
      g.m_operations=ops;
      g.m_variables=vars;
      g.m_wires=wires;
      g.m_ports=ports;
    }

    static void setPrivates
    (minsky::GroupIcon& g, const vector<int>& ops, const vector<int>& vars,
     const vector<int>& wires, const vector<int>& ports)
    {
      ecolab::array<int> p;
      p=toArray(ports);
      setPrivates(g,ops,vars,wires,p);
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
