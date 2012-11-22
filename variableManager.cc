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
#include "variableManager.h"
#include "portManager.h"
#include "minsky.h"
#include <ecolab_epilogue.h>

#include <set>
using namespace std;
using namespace minsky;

namespace 
{
  VariableManager* theVariableManager=&minsky::minsky.variables;
}

VariableManager& minsky::variableManager() {return *theVariableManager;}
void minsky::setVariableManager(VariableManager& pm) {theVariableManager=&pm;}

array<int> VariableManager::visibleVariables() const
{
  array<int> ret;
  for (Variables::const_iterator i=Variables::begin(); i!=Variables::end(); ++i)
    if (i->second->visible && !i->second->m_godley) ret<<=i->first;
  return ret;
}

int VariableManager::addVariable(const VariablePtr& var, int id)
{
  // do not add the variable if already present and not referring to
  // the same variable
  if (varExists(var) && (id==-1 || 
                         var->numPorts() == (*this)[id]->numPorts() && 
                         all(var->ports() != (*this)[id]->ports())))
      return -1;
  if (id==-1)  id=empty()? 0: rbegin()->first+1;
  insert(value_type(id,var));
  if (var->lhs()) portToVariable[var->inPort()]=id;
  portToVariable[var->outPort()]=id;
  if (!values.count(var->name) && !var->name.empty())
    values.insert
      (VariableValues::value_type(var->name, VariableValue(var->type())));
  assert(var->type()==values[var->name].type());

  return id;
}

bool VariableManager::varExists(const VariablePtr& var) const
{
  return portToVariable.count(var->outPort());
}

int VariableManager::newVariable(const string& name)
{
  VariableValues::iterator v=values.find(name);
  if (v==values.end())
    return addVariable(VariablePtr(VariableBase::flow,name));
  else
    return addVariable(VariablePtr(v->second.type(),name));
}

void VariableManager::erase(Variables::iterator it)
{
  portToVariable.erase(it->second->outPort());
  if (it->second->lhs()) portToVariable.erase(it->second->inPort());
  Variables::erase(it);
}

void VariableManager::eraseGodleyVariables(const vector<VariablePtr>& varsToKeep)
{
  // construct set of ids to keep
  set<int> idsToKeep;
  for (size_t i=0; i<varsToKeep.size(); ++i)
    {
      PortMap::iterator id=portToVariable.find(varsToKeep[i]->outPort());
      if (id!=portToVariable.end())
        idsToKeep.insert(id->first);
    }

  vector<int> varsToErase;
  for (Variables::iterator i=Variables::begin(); i!=Variables::end(); ++i)
    if (i->second->m_godley && !idsToKeep.count(i->first))
      varsToErase.push_back(i->first);
  for (size_t i=0; i<varsToErase.size(); ++i)
    erase(varsToErase[i]);
}

void VariableManager::erase(int i)
{
  Variables::iterator it=find(i);
  if (it!=Variables::end()) 
    {
      // see if any other instance of this variable exists
      iterator j;
      for (j=begin(); j!=end(); ++j)
        if (j->second->name == it->second->name && 
            j->second->outPort()!=it->second->outPort())
          break;
      if (j==end()) // didn't find any others
        values.erase(it->second->name);
      portManager().delPort(it->second->outPort());
      if (it->second->lhs()) portManager().delPort(it->second->inPort());
      erase(it);
    }
}

void VariableManager::erase(const VariablePtr& v)
{
  PortMap::const_iterator it=portToVariable.find(v->outPort());
  if (it!=portToVariable.end())
    erase(it->second);
}

int VariableManager::wireToVariable(const string& name) const
{
  if (!InputWired(name)) return -1;
  for (const_iterator i=begin(); i!=end(); ++i)
    if (i->second->inPort()>-1 && i->second->name==name)
      {
        array<int> wires=portManager().WiresAttachedToPort(i->second->inPort());
        if (wires.size()>0) 
          return wires[0];
      }
  return -1;
}

array<int> VariableManager::wiresFromVariable(const string& name) const
{
  array<int> wires;
  for (const_iterator i=begin(); i!=end(); ++i)
    if (i->second->outPort()>-1 && i->second->name==name)
      wires<<=portManager().WiresAttachedToPort(i->second->outPort());
  return wires;
}


void VariableManager::removeVariable(string name)
{
  for (Variables::iterator it=Variables::begin(); it!=Variables::end(); )
    if (it->second->name==name)
      erase(it++);
    else
      ++it;
  values.erase(name);
}     

bool VariableManager::addWire(int from, int to)
{
  PortMap::iterator it=portToVariable.find(to);
  if (it!=portToVariable.end())  
    {
      Variables::iterator v=find(it->second);
      if (v!=Variables::end())
        // cannot have more than one wire to an input, nor self-wire a variable
        if (from==v->second->outPort())
          return false;
        else
          return wiredVariables.insert(v->second->name).second;
    }
  return true;
}

void VariableManager::deleteWire(int port)
{
  PortMap::iterator it=portToVariable.find(port);
  if (it!=portToVariable.end())
    {
      Variables::iterator v=find(it->second);
      if (v!=Variables::end())
        wiredVariables.erase(v->second->name);
    }
}

VariablePtr VariableManager::getVariableFromPort(int port) const
{
  PortMap::const_iterator it=portToVariable.find(port);
  if (it!=portToVariable.end())
    {
      Variables::const_iterator v=find(it->second);
      if (v!=Variables::end())
        return v->second;
    }
  return VariablePtr();
}

void VariableManager::reset()
{
  // reallocate all variables
  ValueVector::stockVars.clear();
  ValueVector::flowVars.clear();
  for (VariableValues::iterator v=values.begin(); v!=values.end(); ++v)
    v->second.allocValue();
}

void VariableManager::makeConsistent()
{
  // remove variableValues not in variables
  set<string> existingNames;
  for (iterator i=begin(); i!=end(); ++i)
    existingNames.insert(i->second->name);
  for (VariableValues::iterator i=values.begin(); i!=values.end(); )
    if (existingNames.count(i->first))
      ++i;
    else
      values.erase(i++);

  // regenerate portToVariable
  portToVariable.clear();
  for (Variables::iterator i=Variables::begin(); i!=Variables::end(); ++i)
    {
      if (i->second->inPort()>-1)
        portToVariable[i->second->inPort()]=i->first;
       if (i->second->outPort()>-1)
        portToVariable[i->second->outPort()]=i->first;
    }

  // regenerated wireVariables
  wiredVariables.clear();
  for (PortManager::Wires::iterator w=portManager().wires.begin();
       w!=portManager().wires.end(); ++w)
    {
      PortMap::iterator v=portToVariable.find(w->second.to);
      if (v!=portToVariable.end())
        wiredVariables.insert((*this)[v->second]->name);
    }
}

void VariableManager::clear()
{
  Variables::clear();
  wiredVariables.clear();
  portToVariable.clear();
  values.clear();
}

string VariableManager::valueNames() const
{
  string names;
  for (VariableValues::const_iterator i=values.begin(); i!=values.end(); ++i)
    names+=" "+i->first;
  return names;
}
