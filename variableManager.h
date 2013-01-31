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
#ifndef VARIABLEMANAGER_H
#define VARIABLEMANAGER_H

#include "variable.h"
#include "variableValue.h"

#include <map>
#include <set>
#include <arrays.h>


namespace minsky
{

  /**
     Variables have certain global constraints. Variables with the same
     name refer to the same numerical value (m_idx are identical), and must
     have only one input across the range of instances of the same name.

     A constant can be a variable with no input
  */
  // public inheritance for debugging, and scripting convenience: should be private
  class VariableManager: public std::map<int,VariablePtr>
  {
  public:
    CLASSDESC_ACCESS(VariableManager);
    friend struct SchemaHelper;
    typedef std::map<int,VariablePtr> Variables;
    typedef std::map<int, int> PortMap; 
    typedef std::set<string> WiredVariables;
    typedef std::map<string, VariableValue> VariableValues;
  
  private:
    WiredVariables wiredVariables; /// variables whose input port is wired
    PortMap portToVariable; /// map of ports to variables
  
    VariableValue undefined;

    void erase(Variables::iterator it);
  public:
    VariableValues values; 

    /// useful for debugging, return list of keys of values
    string valueNames() const;

    /// set of ids of variable that are icons in their own right
    array<int> visibleVariables() const;

    // set/get an initial value
    void setInit(const string& name, double val)
    {if (values.count(name)>0) values[name].init=val;}

    /// add a variable to this manager. if \a id==-1, then use the next available id
    /// @return variable id
    int addVariable(const VariablePtr& var, int id=-1);
    /// returns true if variable has already been added
    bool varExists(const VariablePtr& var) const;
    /// creates a new variable and adds it. If a variable of the same
    /// name already exists, that type is used, otherwise a flow
    /// variable is created.
    int newVariable(const string& name);
    /// remove variable i
    void erase(int i);
    void erase(const VariablePtr&);
    /// erase just those variables with the godley attribute set
    void eraseGodleyVariables(const std::vector<VariablePtr>& varsToKeep);

    bool InputWired(const string& name) const {return wiredVariables.count(name);}
    bool inputWired(TCL_args name) const {return InputWired(name);}
    /// returns wire connecting to the variable
    int wireToVariable(const string& name) const;
    /// returns a list of wires emanating from the variable
    array_ns::array<int> wiresFromVariable(const string& name) const;

    /// TCL helper to check if a variable already exists by the same name
    bool exists(TCL_args args) {return values.count(args);}
    /// remove all instances of variable \a name
    void removeVariable(string name);
    /// returns true if wire can successfully connect to port
    bool addWire(int from, int to);
    /// deletes wire 
    void deleteWire(int port);
    /// return ID for variable owning \a port. Returns -1 if no such
    /// variable is registered.
    int getVariableIDFromPort(int port) const;
    /// return reference to variable owning \a port. Returns a default
    /// constructed VariablePtr if no such variable is registered
    VariablePtr getVariableFromPort(int port) const;
    VariableValue& getVariableValue(const string& name) {
      VariableValues::iterator v=values.find(name);
      if (v!=values.end()) return v->second;
      return undefined;
    }
    VariableValue& getVariableValueFromPort(int port)  {
      return getVariableValue(getVariableFromPort(port)->Name());
    }

    const VariableValue& getVariableValue(const string& name) const {
      VariableValues::const_iterator v=values.find(name);
      if (v!=values.end()) return v->second;
      return undefined;
    }
    const VariableValue& getVariableValueFromPort(int port) const {
      return getVariableValue(getVariableFromPort(port)->Name());
    }
  
    int getIDFromVariable(const VariablePtr& v) const {
      PortMap::const_iterator i=portToVariable.find(v->outPort());
      if (i==portToVariable.end()) return -1;
      else return i->second;
    }
      

    /// reallocates variables in ValueVector, and set value back to init
    void reset();

    /// scans variable, wire & port definitions to correct any inconsistencies
    /// - useful after a load to correct corrupt xml files
    void makeConsistent();

    /// clears all owned data structures
    void clear();
  };

  /// global variablemanager
  VariableManager& variableManager();
}

#ifdef _CLASSDESC
#pragma omit pack minsky::VariableManager::iterator
#pragma omit unpack minsky::VariableManager::iterator
#pragma omit xml_pack minsky::VariableManager::iterator
#pragma omit xml_unpack minsky::VariableManager::iterator
#endif
#include "variableManager.cd"
#endif
