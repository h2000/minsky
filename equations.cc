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

#include "equations.h"
#include "minsky.h"
using namespace minsky;

namespace
{
  // RAII class that conditionally writes a left parenthesis on
  // construction, and right parenthesis on destruction
  struct ParenIf
  {
    ostream& o;
    bool c;
    ParenIf(ostream& o, bool c): o(o), c(c)  {if (c) o<<"\\left(";}
    ~ParenIf() {if (c) o<<"\\right)";}
  };
}

namespace MathDAG
{
  int OperationDAG::BODMASlevel() const
  {
    switch (type)
      {
      case OperationType::time:
      case OperationType::copy:
      case OperationType::integrate:
      case OperationType::exp:
        return 0;
      case OperationType::multiply:
      case OperationType::divide:
        return 1;
      case OperationType::subtract:
      case OperationType::add:
        return 2;
      case OperationType::constant: // varies, depending on what's in it
        if (name.find_first_of("+-")!=string::npos)
          return 2;
        else
          return 1;
      default:
        assert(!"BODMASlevel not defined for type");
      }
    return 10;  // should not be here!
  }

  // named constants for group identities
  struct Zero: public Node
  {
    int BODMASlevel() const {return 0;}
    ostream& latex(ostream& o) const {return o<<"0";}
  } zero;

  struct One: public Node
  {
    int BODMASlevel() const {return 0;}
    ostream& latex(ostream& o) const {return o<<"1";}
  } one;

  // wraps in \mathrm if nm has more than one letter - and also takes
  // into account LaTeX subscript/superscripts (TODO)
  string mathrm(const string& nm)
  {
    // process super/sub scripts
    string::size_type ss;
    if ((ss=nm.find_first_of("_^"))!=string::npos)
      return mathrm(nm.substr(0, ss)) + nm[ss] + mathrm(nm.substr(ss+1));

    // if its a single letter variable, or contains LaTeX codes, process as is
    if (nm.length()==1 || nm.find('\\')!=string::npos)
      return nm;
    else
      return "\\mathrm{"+nm+"}";
  }

  ostream& VariableDAG::latex(ostream& o) const
  {
    return o<<mathrm(name);
  }

  ostream& OperationDAG::latex(ostream& o) const
  {
    switch (type)
      {

      case OperationType::constant:
        o<<mathrm(name);
        break;

      case OperationType::time:
        o<<" t ";
        break;
      case OperationType::exp:
        assert(arguments.size()>=1);
        if (arguments[0].size()>=1)
          {
            assert(arguments[0][0]);
            o<<"\\exp\\left("<<arguments[0][0]->latex()<<"\\right)";
          }
        break;

      case OperationType::integrate:
        assert(!"integration should be turned into a derivative!");
        break;
        
      case OperationType::add:
        assert(arguments.size()>=2);
        for (size_t i=0; i<arguments[0].size(); ++i)
          {
            assert(arguments[0][i]);
            if (i>0) o<<"+";
            o<<arguments[0][i]->latex();
          }
        if (arguments[0].size()>0 && arguments[1].size()) o<<"+";
        for (size_t i=0; i<arguments[1].size(); ++i)
          {
            assert(arguments[1][i]);
            if (i>0) o<<"+";
            o<<arguments[1][i]->latex();
          }
         break;

      case OperationType::subtract:
        assert(arguments.size()>=2);
        for (size_t i=0; i<arguments[0].size(); ++i)
          {
            assert(arguments[0][i]);
            if (i>0) o<<"+";
            o<<arguments[0][i]->latex();
          }
        if (arguments[1].size()>0) 
          {
            o<<"-";
            ParenIf p(o, (arguments[1].size()>1 || 
                          BODMASlevel() == arguments[1][0]->BODMASlevel()));
            for (size_t i=0; i<arguments[1].size(); ++i)
              {
                assert(arguments[1][i]);
                if (i>0) o<<"+";
                o<<arguments[1][i]->latex();
              }
          }
         break;
        
      case OperationType::multiply:
        assert(arguments.size()>=2);
        for (size_t i=0; i<arguments[0].size(); ++i)
          {
            assert(arguments[0][i]);
            if (i>0) o<<"\\times ";
            ParenIf p(o, arguments[0][i]->BODMASlevel()>BODMASlevel());
            o<<arguments[0][i]->latex();
          }
        if (arguments[0].size()>0 && arguments[1].size()>0) o<<"\\times ";
        for (size_t i=0; i<arguments[1].size(); ++i)
          {
            assert(arguments[1][i]);
            if (i>0) o<<"\\times ";
            ParenIf p(o, arguments[1][i]->BODMASlevel()>BODMASlevel());
            o<<arguments[1][i]->latex();
          }
         break;

      case OperationType::divide:
        assert(arguments.size()>=2);
        o<< "\\frac{";
        if (arguments[0].size()==0) o<<"1";
        for (size_t i=0; i<arguments[0].size(); ++i)
          {
            assert(arguments[0][i]);
            if (i>0) o<<"\\times ";
            ParenIf p(o, i>0 && arguments[0][i]->BODMASlevel()>BODMASlevel());
            o<<arguments[0][i]->latex();
          }
        o<<"}{";
        if (arguments[1].size()==0) o<<"1";
        for (size_t i=0; i<arguments[1].size(); ++i)
          {
            assert(arguments[1][i]);
            if (i>0) o<<"\\times ";
            ParenIf p(o, i>0 && arguments[0][i]->BODMASlevel()>BODMASlevel());
            o<<arguments[1][i]->latex();
          }
        o<<"}";
        break;
    

      default:
        assert(false);
        break;
      }

    return o;
  }

  ostream& GodleyColumnDAG::latex(ostream& o) const
  {
    for (const_iterator i=begin(); i!=end(); ++i)
      {
        if ((*i)[0]=='-') 
          o<<'-'<<mathrm(i->substr(1));
        else
          {
            if (i!=begin())
              o<<'+';
            o<<mathrm(*i);
          }
      }
    return o; 
  }

  SystemOfEquations::SystemOfEquations(const Minsky& m):
    vm(m.variables), ops(m.operations), pm(m)
  {
    // firstly, we need to create a map of ports belonging to operations
    for (Operations::const_iterator o=ops.begin(); o!=ops.end(); ++o)
      for (int i=0; i<o->second->numPorts(); ++i)
        portToOperation[o->second->ports()[i]]=o->first;

    // search through operations looking for integrals
    for (Operations::const_iterator o=ops.begin(); o!=ops.end(); ++o)
      if (IntOp* i=dynamic_cast<IntOp*>(o->second.get()))
        {
          
          integrationVariables.push_back
            (VariableDAG(i->description()));
          assert(pm.WiresAttachedToPort(i->ports()[1]).size()==1);
          integrationVariables.back().rhs.reset
            (createNodeFromWire(pm.WiresAttachedToPort(i->ports()[1])[0]));
        }

    // process the Godley tables
    map<string, GodleyColumnDAG> godleyVars;
    for (Minsky::GodleyItems::const_iterator g=m.godleyItems.begin(); 
         g!=m.godleyItems.end(); ++g)
      processGodleyTable(godleyVars, g->second.table);

    for (map<string, GodleyColumnDAG>::iterator v=godleyVars.begin(); 
         v!=godleyVars.end(); ++v)
      {
        integrationVariables.push_back(VariableDAG(v->first));
        integrationVariables.back().rhs.reset(new GodleyColumnDAG(v->second));
      }

    // now start with the variables, and work our way back to how they
    // are defined
    for (VariableManager::VariableValues::const_iterator v=vm.values.begin();
         v!=vm.values.end(); ++v)
      if (v->second.lhs())
        variables.push_back(makeDAG(v->first));

  }

  VariableDAG SystemOfEquations::makeDAG(const string& name)
  {
    VariableDAG r(name);
    r.rhs.reset(createNodeFromWire(vm.wireToVariable(name)));
    return r;
  }

  OperationDAG SystemOfEquations::makeDAG(const OperationBase& op)
  {
    string description;
    if (const Constant* c=dynamic_cast<const Constant*>(&op))
      description=c->description;
    else if (const IntOp* i=dynamic_cast<const IntOp*>(&op))
      description=i->getDescription();

    OperationDAG r(op.type(), description);
    r.arguments.resize(op.numPorts()-1);
    for (size_t i=1; i<op.numPorts(); ++i)
      {
        PortManager::Ports::const_iterator p=pm.ports.find(op.ports()[i]);
        if (p != pm.ports.end() && p->second.input)
          {
            array<int> wires=pm.WiresAttachedToPort(op.ports()[i]);
            for (int w=0; w<wires.size(); ++w)
              r.arguments[i-1].push_back
                (shared_ptr<Node>(createNodeFromWire(wires[w])));
          }
      }
    return r;
  }

  Node* SystemOfEquations::createNodeFromWire(int inputWire)
  {
    PortManager::Wires::const_iterator wi=pm.wires.find(inputWire);
    if (wi != pm.wires.end())
      {
        const Wire& w=wi->second;
        VariablePtr v(vm.getVariableFromPort(w.from));
        if (v && v->type()!=VariableBase::undefined) 
          // see whether we're wired to a variable
          return new VariableDAG(makeDAG(v->name));
        else if (portToOperation.count(w.from))
          // we're wired to an operation
          return new OperationDAG
            (makeDAG(*ops.find(portToOperation[w.from])->second));
      }
  return NULL;
}

  ostream& SystemOfEquations::latex(ostream& o) const
  {
    o << "\\begin{eqnarray*}\n";
    for (vector<VariableDAG>::const_iterator i=variables.begin(); 
         i!=variables.end(); ++i)
      {
        o << i->latex() << "&=&";
        if (i->rhs) 
          i->rhs->latex(o);
        o << "\\\\\n";
      }

    for (vector<VariableDAG>::const_iterator i=integrationVariables.begin(); 
         i!=integrationVariables.end(); ++i)
      {
        o << "\\frac{ d " << mathrm(i->name) << 
          "}{dt} &=&";
        if (i->rhs)
          i->rhs->latex(o);
        o << "\\\\\n";
      }
    return o << "\\end{eqnarray*}\n";
  }

  void SystemOfEquations::processGodleyTable
    (map<string, GodleyColumnDAG>& godleyVariables, const GodleyTable& godley)
  {
    for (int c=1; c<godley.cols(); ++c)
      {
        string name=godley.cell(0,c);
        stripNonAlnum(name);
        GodleyColumnDAG& gd=godleyVariables[name];

        for (int r=1; r<godley.rows(); ++r)
          {
            if (godley.initialConditionRow(r)) continue;
            const string& formula = godley.cell(r,c);
            size_t start=0;
            while (start<formula.length() && isspace(formula[start])) start++;
            if (start<formula.length())
              {
                // for the moment, only deal with signed variables
                string name=formula; stripNonAlnum(name);
                if (godley.signConventionReversed(c))
                  // we must reverse the sign convention
                  if (formula[start]=='-')
                    gd.push_back(name);
                  else
                    gd.push_back("-"+name);
                else
                  if (formula[start]=='-')
                    gd.push_back("-"+name);
                  else
                    gd.push_back(name);
                }
            }
      }
  }
}


