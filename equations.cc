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
#include "str.h"
#include <ecolab_epilogue.h>
using namespace minsky;

namespace MathDAG
{

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

    struct InvalidChar
    {
      bool operator()(char c) const {return !isalnum(c) && c!='_';}
    };

    string validMatlabIdentifier(string name)
    {
      name.erase(remove_if(name.begin(), name.end(), InvalidChar()), name.end());
      if (name.empty() || isdigit(name[0]))
        name="_"+name;
      return name;
    }

    // comparison function for sorting variables into their definition order
    struct VariableDefOrder
    {
      bool operator()(const VariableDAG& x, const VariableDAG& y) {
        return x.order()<y.order();
      }
    };

  }

  // named constants for group identities
  struct Zero: public Node
  {
    int BODMASlevel() const {return 0;}
    ostream& latex(ostream& o) const {return o<<"0";}
    ostream& matlab(ostream& o) const {return o<<"0";}
    int order() const {return 0;}
  } zero;

  struct One: public Node
  {
    int BODMASlevel() const {return 0;}
    ostream& latex(ostream& o) const {return o<<"1";}
    ostream& matlab(ostream& o) const {return o<<"1";}
    int order() const {return 0;}
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

  string latex(double x)
  {
    if (abs(x)>0 && (abs(x)>=1e5 || abs(x)<=1e-5))
      {
        int exponent=log10(abs(x));
        if (exponent<0) exponent++;
        return str(x/pow(10,exponent))+"\\times10^{"+str(exponent)+"}";
      }
    else
      return str(x);
  }

  ostream& VariableDAG::latex(ostream& o) const
  {
    return o<<mathrm(name);
  }

  ostream& VariableDAG::matlab(ostream& o) const
  {
    return o<<validMatlabIdentifier(name);
  }

  OperationDAGBase* OperationDAGBase::create(Type type, const string& name)
  {
    switch (type)
      {
      case constant: return new OperationDAG<constant>(name);
      case add: return new OperationDAG<add>(name);
      case subtract: return new OperationDAG<subtract>(name);
      case multiply: return new OperationDAG<multiply>(name);
      case divide: return new OperationDAG<divide>(name);
      case log: return new OperationDAG<log>(name);
      case pow: return new OperationDAG<pow>(name);
      case time: return new OperationDAG<time>(name);
      case copy: return new OperationDAG<copy>(name);
      case integrate: return new OperationDAG<integrate>(name);
      case sqrt: return new OperationDAG<sqrt>(name);
      case exp: return new OperationDAG<exp>(name);
      case ln: return new OperationDAG<ln>(name);
      case sin: return new OperationDAG<sin>(name);
      case cos: return new OperationDAG<cos>(name);
      case tan: return new OperationDAG<tan>(name);
      case asin: return new OperationDAG<asin>(name);
      case acos: return new OperationDAG<acos>(name);
      case atan: return new OperationDAG<atan>(name);
      case sinh: return new OperationDAG<sinh>(name);
      case cosh: return new OperationDAG<cosh>(name);
      case tanh: return new OperationDAG<tanh>(name);
      default: 
        throw error("invalid operation type %s,",typeName(type).c_str());
      }
  }

  // the definition order of an operation is simply the maximum of all
  // of its arguments
  int OperationDAGBase::order() const
  {
    // constants have order one, as they must be ordered after the
    // "fake" variables have been initialised
    int order=type()==constant? 1: 0;
    for (size_t i=0; i<arguments.size(); ++i)
      for (size_t j=0; j<arguments[i].size(); ++j)
        {
          assert(arguments[i][j]);
          order=max(order, arguments[i][j]->order());
        }
    return order;
  }

  template <>
  ostream& OperationDAG<OperationType::constant>::matlab(ostream& o) const
  {
    return o<<init;
  }

  template <>
  ostream& OperationDAG<OperationType::add>::matlab(ostream& o) const
  {
    assert(arguments.size()>=2);
    for (size_t i=0; i<arguments[0].size(); ++i)
      {
        assert(arguments[0][i]);
        if (i>0) o<<"+";
        o<<arguments[0][i]->matlab();
      }
    if (arguments[0].size()>0 && arguments[1].size()) o<<"+";
    for (size_t i=0; i<arguments[1].size(); ++i)
      {
        assert(arguments[1][i]);
        if (i>0) o<<"+";
        o<<arguments[1][i]->matlab();
      }
    return o;
  }

  template <>
  ostream& OperationDAG<OperationType::subtract>::matlab(ostream& o) const
  {
    assert(arguments.size()>=2);
    for (size_t i=0; i<arguments[0].size(); ++i)
      {
        assert(arguments[0][i]);
        if (i>0) o<<"+";
        o<<arguments[0][i]->matlab();
      }
    if (arguments[1].size()>0) 
      {
        o<<"-(";
        for (size_t i=0; i<arguments[1].size(); ++i)
          {
            assert(arguments[1][i]);
            if (i>0) o<<"+";
            o<<arguments[1][i]->matlab();
          }
        o<<")";
      }
    return o;
  }
        
  template <>
  ostream& OperationDAG<OperationType::multiply>::matlab(ostream& o) const
  {
    assert(arguments.size()>=2);
    for (size_t i=0; i<arguments[0].size(); ++i)
      {
        assert(arguments[0][i]);
        if (i>0) o<<"*";
        o<<"("<<arguments[0][i]->matlab()<<")";
      }
    if (arguments[0].size()>0 && arguments[1].size()>0) o<<"*";
    for (size_t i=0; i<arguments[1].size(); ++i)
      {
        assert(arguments[1][i]);
        if (i>0) o<<"*";
        o<<"("<<arguments[1][i]->matlab()<<")";
      }
    return o;
  }

  template <>
  ostream& OperationDAG<OperationType::divide>::matlab(ostream& o) const
  {
    assert(arguments.size()>=2);
    if (arguments[0].size()==0) 
      o<<"1";
    for (size_t i=0; i<arguments[0].size(); ++i)
      {
        assert(arguments[0][i]);
        if (i>0) o<<"*";
        o<<"("<<arguments[0][i]->matlab()<<")";
      }
    if (arguments[1].size()>0) 
      {
        o<<"/(";
        for (size_t i=0; i<arguments[1].size(); ++i)
          {
            assert(arguments[1][i]);
            if (i>0) o<<"*";
            o<<"("<<arguments[1][i]->matlab()<<")";
          }
        o<<")";
      }
    return o;
  }
  
  template <>
  ostream& OperationDAG<OperationType::log>::matlab(ostream& o) const
  {
    assert(arguments.size()==2 && arguments[0].size()==1 && arguments[1].size()==1);
    return o<<"log("<<arguments[0][0]->matlab()<<")/log("<<
      arguments[1][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::pow>::matlab(ostream& o) const
  {
    assert(arguments.size()==2 && arguments[0].size()==1 && arguments[1].size()==1);
    return  o<<"("<<arguments[0][0]->matlab()<<")^("<<arguments[1][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::time>::matlab(ostream& o) const
  {
    return o<<"t";
  }

  template <>
  ostream& OperationDAG<OperationType::copy>::matlab(ostream& o) const
  {
    assert(arguments.size()>=1);
    if (arguments[0].size()>=1)
      {
        assert(arguments[0][0]);
        o<<arguments[0][0]->matlab();
      }
    return o;
  }

  template <>
  ostream& OperationDAG<OperationType::integrate>::matlab(ostream& o) const
  {
    assert(!"integration should be turned into a derivative!");
    return o;
  }
        
  template <>
  ostream& OperationDAG<OperationType::sqrt>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"sqrt("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::exp>::matlab(ostream& o) const
  {
    assert(arguments.size()>=1);
    if (arguments[0].size()>=1)
      {
        assert(arguments[0][0]);
        o<<"exp("<<arguments[0][0]->matlab()<<")";
      }
    return o;
  }

  template <>
  ostream& OperationDAG<OperationType::ln>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"log("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::sin>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"sin("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::cos>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"cos("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::tan>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"tan("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::asin>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"asin("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::acos>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"acos("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::atan>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"atan("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::sinh>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"sinh("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::cosh>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"cosh("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::tanh>::matlab(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"tanh("<<arguments[0][0]->matlab()<<")";
  }

  template <>
  ostream& OperationDAG<OperationType::constant>::latex(ostream& o) const
  {
    return o<<mathrm(name);
  }

  template <>
  ostream& OperationDAG<OperationType::add>::latex(ostream& o) const
  {
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
    return o;
  }

  template <>
  ostream& OperationDAG<OperationType::subtract>::latex(ostream& o) const
  {
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
    return o;
  }
        
  template <>
  ostream& OperationDAG<OperationType::multiply>::latex(ostream& o) const
  {
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
    return o;
  }

  template <>
  ostream& OperationDAG<OperationType::divide>::latex(ostream& o) const
  {
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
    return o<<"}";
  }
  
  template <>
  ostream& OperationDAG<OperationType::log>::latex(ostream& o) const
  {
    assert(arguments.size()==2 && arguments[0].size()==1 && arguments[1].size()==1);
    return o<<"\\log_{"<<arguments[1][0]->latex()<<"}\\left("<<
      arguments[0][0]->latex()<<"\\right)";
  }

  template <>
  ostream& OperationDAG<OperationType::pow>::latex(ostream& o) const
  {
    assert(arguments.size()==2 && arguments[0].size()==1 && arguments[1].size()==1);
    {
      ParenIf p(o, arguments[0][0]->BODMASlevel()>BODMASlevel());
      o<<arguments[0][0];
    }
    return o<<"^{"<<arguments[1][0]->latex()<<"}";
  }

  template <>
  ostream& OperationDAG<OperationType::time>::latex(ostream& o) const
  {
    return o<<" t ";
  }

  template <>
  ostream& OperationDAG<OperationType::copy>::latex(ostream& o) const
  {
    assert(arguments.size()>=1);
    if (arguments[0].size()>=1)
      {
        assert(arguments[0][0]);
        o<<arguments[0][0]->latex();
      }
    return o;
  }

  template <>
  ostream& OperationDAG<OperationType::integrate>::latex(ostream& o) const
  {
    assert(!"integration should be turned into a derivative!");
    return o;
  }
        
  template <>
  ostream& OperationDAG<OperationType::sqrt>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\sqrt{"<<arguments[0][0]->latex()<<"}";
  }

  template <>
  ostream& OperationDAG<OperationType::exp>::latex(ostream& o) const
  {
    assert(arguments.size()>=1);
    if (arguments[0].size()>=1)
      {
        assert(arguments[0][0]);
        o<<"\\exp\\left("<<arguments[0][0]->latex()<<"\\right)";
      }
    return o;
  }

  template <>
  ostream& OperationDAG<OperationType::ln>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\ln\\left("<<arguments[0][0]->latex()<<"\\right)";
  }

  template <>
  ostream& OperationDAG<OperationType::sin>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\sin\\left("<<arguments[0][0]->latex()<<"\\right)";
  }

  template <>
  ostream& OperationDAG<OperationType::cos>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\cos\\left("<<arguments[0][0]->latex()<<"\\right)";
  }

  template <>
  ostream& OperationDAG<OperationType::tan>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\tan\\left("<<arguments[0][0]->latex()<<"\\right)";
  }

  template <>
  ostream& OperationDAG<OperationType::asin>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\arcsin\\left("<<arguments[0][0]->latex()<<"\\right)";
  }

  template <>
  ostream& OperationDAG<OperationType::acos>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\arccos\\left("<<arguments[0][0]->latex()<<"\\right)";
  }

  template <>
  ostream& OperationDAG<OperationType::atan>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\arctan\\left("<<arguments[0][0]->latex()<<"\\right)";
  }

  template <>
  ostream& OperationDAG<OperationType::sinh>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\sinh\\left("<<arguments[0][0]->latex()<<"\\right)";
  }

  template <>
  ostream& OperationDAG<OperationType::cosh>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\cosh\\left("<<arguments[0][0]->latex()<<"\\right)";
  }

  template <>
  ostream& OperationDAG<OperationType::tanh>::latex(ostream& o) const
  {
    assert(arguments.size()==1 && arguments[0].size()==1);
    return o<<"\\tanh\\left("<<arguments[0][0]->latex()<<"\\right)";
  }

  ostream& GodleyColumnDAG::matlab(ostream& o) const
  {
    for (const_iterator i=begin(); i!=end(); ++i)
      {
        if ((*i)[0]=='-') 
          o<<'-'<<validMatlabIdentifier(i->substr(1));
        else
          {
            if (i!=begin())
              o<<'+';
            o<<validMatlabIdentifier(*i);
          }
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

    // store stock & integral variables for later reordering
    map<string, VariableDAG> integVarMap;

    // search through operations looking for integrals
    for (Operations::const_iterator o=ops.begin(); o!=ops.end(); ++o)
      if (IntOp* i=dynamic_cast<IntOp*>(o->second.get()))
        {
          
          VariableDAG& v=integVarMap[i->description()];
          v=makeDAG(i->description());
          assert(pm.WiresAttachedToPort(i->ports()[1]).size()==1);
          v.rhs.reset
            (createNodeFromWire(pm.WiresAttachedToPort(i->ports()[1])[0]));
        }
      else if (Constant* c=dynamic_cast<Constant*>(o->second.get()))
        {
          variables.push_back(VariableDAG(c->description));
          variables.back().rhs.reset(new ConstantDAG(c->value));
        }

    // process the Godley tables
    map<string, GodleyColumnDAG> godleyVars;
    for (Minsky::GodleyItems::const_iterator g=m.godleyItems.begin(); 
         g!=m.godleyItems.end(); ++g)
      processGodleyTable(godleyVars, g->second.table);

    for (map<string, GodleyColumnDAG>::iterator g=godleyVars.begin(); 
         g!=godleyVars.end(); ++g)
      {
        VariableDAG& v=integVarMap[g->first]=makeDAG(g->first);
        v.rhs.reset(new GodleyColumnDAG(g->second));
      }

    // reorder integration variables according to lhsVars
    const vector<string>& sVars=vm.stockVars();
    for (vector<string>::const_iterator v=sVars.begin(); 
         v!=sVars.end(); ++v)
      integrationVariables.push_back(integVarMap[*v]);

    // now start with the variables, and work our way back to how they
    // are defined
    for (VariableManager::VariableValues::const_iterator v=vm.values.begin();
         v!=vm.values.end(); ++v)
      if (v->second.lhs())
        variables.push_back(makeDAG(v->first));

    // sort variables into their order of definition
    sort(variables.begin(), variables.end(), VariableDefOrder());
    assert(integrationVariables.size()==vm.stockVars().size());
  }

  VariableDAG SystemOfEquations::makeDAG(const string& name)
  {
    VariableDAG r(name);
    VariableValue vv=vm.getVariableValue(name);
    r.init=vv.init;
    if (vv.lhs()) 
      {
        r.rhs.reset(createNodeFromWire(vm.wireToVariable(name)));
        if (!r.rhs) // add initial condition
          r.rhs.reset(new ConstantDAG(r.init));
      }
    return r;
  }

  OperationDAGBase* SystemOfEquations::makeDAG(const OperationBase& op)
  {
    OperationDAGBase* r=OperationDAGBase::create(op.type());
    if (const Constant* c=dynamic_cast<const Constant*>(&op))
      {
        r->name=c->description;
        r->init=c->value;
      }
    else if (const IntOp* i=dynamic_cast<const IntOp*>(&op))
      r->name=i->getDescription();

    r->arguments.resize(op.numPorts()-1);
    for (size_t i=1; i<op.numPorts(); ++i)
      {
        PortManager::Ports::const_iterator p=pm.ports.find(op.ports()[i]);
        if (p != pm.ports.end() && p->second.input)
          {
            array<int> wires=pm.WiresAttachedToPort(op.ports()[i]);
            for (int w=0; w<wires.size(); ++w)
              r->arguments[i-1].push_back
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
          return new VariableDAG(makeDAG(v->Name()));
        else if (portToOperation.count(w.from))
          // we're wired to an operation
          return makeDAG(*ops.find(portToOperation[w.from])->second);
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
        o << mathrm(i->name)<<"(0)&=&"<<MathDAG::latex(i->init)<<"\\\\\n";
        o << "\\frac{ d " << mathrm(i->name) << 
          "}{dt} &=&";
        if (i->rhs)
          i->rhs->latex(o);
        o << "\\\\\n";
      }
    return o << "\\end{eqnarray*}\n";
  }

  ostream& SystemOfEquations::matlab(ostream& o) const
  {
    assert(integrationVariables.size()==vm.stockVars().size());
    o<<"function f=f(x,t)\n";
    // define names for the components of x for reference
    int j=1;
    for (vector<VariableDAG>::const_iterator i=integrationVariables.begin(); 
         i!=integrationVariables.end(); ++i,++j)
      o<<i->matlab()<<"=x("<<j<<");\n";

    for (vector<VariableDAG>::const_iterator i=variables.begin(); 
         i!=variables.end(); ++i)
      if (i->rhs)
        o << i->matlab() << "="<<i->rhs->matlab()<<";\n";

    j=1;
    for (vector<VariableDAG>::const_iterator i=integrationVariables.begin(); 
         i!=integrationVariables.end(); ++i, ++j)
      {
        o << "f("<<j<<")=";
        if (i->rhs)
          i->rhs->matlab(o);
        else
          o<<0;
        o<<";\n";
      }
    o<<"endfunction;\n\n";

    // now write out the initial conditions
    j=1;
    for (vector<VariableDAG>::const_iterator i=integrationVariables.begin(); 
         i!=integrationVariables.end(); ++i, ++j)
      o << "x0("<<j<<")="<<i->init<<";\n";
   
    return o;
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


