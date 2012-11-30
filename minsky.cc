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
#include "classdesc_access.h"
#include <ecolab.h>
#include "TCL_obj_stl.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_odeiv2.h>

#include "minsky.h"

#include <schema/schema0.h>
#include <schema/schema1.h>

using namespace minsky;
using namespace classdesc;

namespace
{
  const char* schemaURL="http://minsky.sf.net/minsky";

  /*
    For using GSL Runge-Kutta routines
  */

  int function(double t, const double y[], double f[], void *params)
  {
    if (params==NULL) return GSL_FAILURE;
    ((Minsky*)params)->evalEquations(f,y);
    return GSL_SUCCESS;
  }

  int jacobian(double t, const double y[], double * dfdy, double dfdt[], void * params)
  {
    if (params==NULL) return GSL_FAILURE;
    Minsky::Matrix jac(ValueVector::stockVars.size(), dfdy);
    ((Minsky*)params)->jacobian(jac,y);
    return GSL_SUCCESS;
  }
}

namespace minsky
{

  struct RKdata
  {
    gsl_odeiv2_system sys;
    gsl_odeiv2_driver* driver;
    RKdata(Minsky* minsky) {
      sys.function=function;
      sys.jacobian=jacobian;
      sys.dimension=ValueVector::stockVars.size();
      sys.params=minsky;
      driver = gsl_odeiv2_driver_alloc_y_new
        (&sys, gsl_odeiv2_step_rkf45, minsky->stepMax, minsky->epsAbs, 
         minsky->epsRel);
      gsl_odeiv2_driver_set_hmax(driver, minsky->stepMax);
      gsl_odeiv2_driver_set_hmin(driver, minsky->stepMin);
    }
    ~RKdata() {gsl_odeiv2_driver_free(driver);}
  };
}

#include "minsky.cd"
#include "minskyVersion.h"

#include <ecolab_epilogue.h>


#include <algorithm>
using namespace std;

namespace minsky
{
  Minsky minsky;
  make_model(minsky);
}

Minsky::Minsky(): edited(true), 
                  port(ports), wire(wires), op(operations), 
                  constant(operations), integral(operations), var(variables),
                  value(variables.values), plot(plots.plots), 
                  godleyItem(godleyItems), groupItem(groupItems),
                  t(0), stepMin(0), stepMax(1), nSteps(1),
                  epsAbs(1e-3), epsRel(1e-2) 
{
}

void Minsky::clearAll()
{
  wires.clear(); 
  ports.clear();
  godleyItems.clear();
  operations.clear();
  variables.clear();
  groupItems.clear();
  plots.clear();
}


const char* Minsky::minskyVersion=MINSKY_VERSION;

bool Minsky::portInput(TCL_args args)
{
  int id=args;
  assert(ports.count(id));
  return ports[id].input;
}

int Minsky::addWire(TCL_args args) {
  int from=args, to=args;
  // wire must go from an output port to an input port
  if (ports[from].input || !ports[to].input)
    return -1;

  // check we're not wiring an operator to its input
  // TODO: move this into an operationManager class
  // TODO: check that multiple input wires are only to binary ops.
  for (Operations::const_iterator o=operations.begin(); 
       o!=operations.end(); ++o)
    if (o->second->selfWire(from, to))
      return -1;

  // check whether variable manager will allow the connection
  if (!variables.addWire(from, to))
    return -1;

  Wire w(from, to);
  args>>w.coords;
  if (w.coords.size()<4)
    return -1;
  edited = true;
  return PortManager::addWire(w);
}

void Minsky::deleteWire(TCL_args args)
{
  int id=args;
  if (wires.count(id))
    {
      variables.deleteWire(wires[id].to);
      PortManager::deleteWire(id);
    }
}

array<float> Minsky::wireCoords(TCL_args args)
{
  int id=args;
  assert(wires.count(id));
  Wire& wire=wires[id];
  if (args.count>0)
    {
      array<float> tmp;
      args>>tmp;
      if (tmp.size()>=4)
        wire.coords=tmp;
    }
  assert(wire.coords.size()>=4);
  return wire.coords;
}



int Minsky::AddOperation(const char* o)
{
  OperationPtr newOp(static_cast<OperationType::Type>
                     (enumKey<OperationType::Type>(o)));
  if (!newOp) return -1;
  int id=operations.empty()? 0: operations.rbegin()->first+1;
  operations.insert(make_pair(id, newOp));
  edited = true;
  return id;
}

int Minsky::CopyOperation(int id)
{
  Operations::iterator source=operations.find(id);
  if (source==operations.end()) return -1;
  int newId=operations.empty()? 0: operations.rbegin()->first+1;
  OperationPtr newOp = source->second->clone();
  // newOp->setDescription(source->second->description());
  //  newOp.addPorts();
  operations.insert(make_pair(newId, newOp));
  return newId;
}


void Minsky::DeleteOperation(int opid)
{
  Operations::iterator op=operations.find(opid);
  if (op!=operations.end())
    {
      // delete all wires attached to this operation
      for (int p=0; p<op->second->numPorts(); ++p)
        {
          array<int> wires=WiresAttachedToPort(op->second->ports()[p]);
          for (int w=0; w<wires.size(); ++w)
            {
              int wid=wires[w];
              variables.deleteWire(PortManager::wires[wid].to);
              PortManager::deleteWire(wid);
            }
        }
      operations.erase(op);
      edited = true;
    }
}

int Minsky::group(TCL_args args)
{
  int id=groupItems.empty()? 0: groupItems.rbegin()->first+1;
  groupItems[id].group(args[0], args[1], args[2], args[3], id);
  return id;
}

void Minsky::ungroup(TCL_args args)
{
  int id=args;
  groupItems[id].ungroup();
  groupItems.erase(id);
}

int Minsky::CopyGroup(int id)
{
  GroupIcons::iterator srcIt=groupItems.find(id);
  if (srcIt==groupItems.end()) return -1; //src not found
  int newId=groupItems.rbegin()->first+1;
  groupItems[newId].copy(srcIt->second);
  return newId;
}

array<int> Minsky::unwiredOperations()
{
  array<int> ret;
  for (Operations::iterator op=operations.begin(); op!=operations.end(); ++op)
    for (int i=0; i<op->second->numPorts(); ++i)
      if (WiresAttachedToPort(op->second->ports()[i]).size()==0)
        {
          ret<<=op->first;
          break;
        }
  return ret;
}

int Minsky::CopyVariable(int id)
{
  if (variables.count(id)>0)
    {
      VariablePtr v(variables[id]->clone());
      v->visible=true; // a copied variable should always be visible!
      return variables.addVariable(v);
    }
  else
    return -1;
}


// sv assume zeroed before call
void Minsky::godleyEval(double sv[], const double fv[])
{
#ifndef NDEBUG
  for (int i=0; i<stockVars.size(); ++i)
    assert(sv[i]==0);
#endif
  for (GodleyItems::iterator gi=godleyItems.begin(); 
       gi!=godleyItems.end(); ++gi)
    {
      GodleyTable& godley=gi->second.table;
      for (int c=1; c<godley.cols(); ++c)
        {
          string name=godley.cell(0,c);
          stripNonAlnum(name);
          VariableValue& stockVar=variables.getVariableValue(name);
          if (stockVar.idx()<0) continue; //variable undefined
          assert(stockVar.idx()<stockVars.size());

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
                  VariableValue& var=variables.getVariableValue(name);
                  if (var.idx()<0) continue;
                  assert(var.idx()<flowVars.size());
                  if (godley.signConventionReversed(c))
                    // we must reverse the sign convention
                    if (formula[start]=='-')
                      sv[stockVar.idx()]+=fv[var.idx()];
                    else
                      sv[stockVar.idx()]-=fv[var.idx()];
                  else
                    if (formula[start]=='-')
                      sv[stockVar.idx()]-=fv[var.idx()];
                    else
                      sv[stockVar.idx()]+=fv[var.idx()];
                }
            }
        }
    }
}


void Minsky::garbageCollect()
{
  stockVars.clear();
  flowVars.clear();
  map<int, Port> oldPortMap;
  oldPortMap.swap(ports);

  // remove all temporaries
  for (VariableManager::VariableValues::iterator v=variables.values.begin(); 
       v!=variables.values.end();)
    if (v->second.temp())
      variables.values.erase(v++);
    else\
      ++v;

  for (VariableManager::iterator v=variables.begin(); 
       v!=variables.end(); ++v)
    {
      // copy used ports into the new port map.
      const array<int>& pp=v->second->ports();
      for (array<int>::const_iterator p=pp.begin(); p!=pp.end(); p++)
        ports[*p] = oldPortMap[*p];
    }

  for (Operations::const_iterator o=operations.begin(); 
       o!=operations.end(); ++o)
    for (int p=0; p<o->second->numPorts(); ++p)
      ports[o->second->ports()[p]] = oldPortMap[o->second->ports()[p]];

  for (Plots::Map::iterator pl=plots.plots.begin(); pl!=plots.plots.end(); ++pl)
    for (int p=0; p<pl->second.ports.size(); ++p)
      ports[pl->second.ports[p]] = oldPortMap[pl->second.ports[p]];

  variables.reset();
  edited = true;
}

namespace {

  // perform depth first ranking of nodes in a graph by level. Graph
  // is assumed to be acyclic
  struct OperationOrderer
  {
    map<int,int> opOrder; // order by operation
    map<int,vector<int> > links; // links in the execution graph
    set<int> visited; //keep track of nodes in stack

    void order(int node, int level) 
    {
      int& currentLevel = opOrder[node];
      if (currentLevel < level)
        currentLevel=level;
      // check for cycles
      if (!visited.insert(node).second)
        throw error("cyclic network detected");
      vector<int>& ll=links[node];
      for (vector<int>::iterator l=ll.begin(); l!=ll.end(); ++l)
        order(*l, level+1);
      visited.erase(node);
    }
  };

  struct OrderSecond
  {
    bool operator()(const pair<int,int>& x, const pair<int,int>& y) const
    {return x.second<y.second;}
  };

  // a convenience class for creating a copy EvalOp
  struct Copy: public EvalOp
  {
    Copy(const VariableValue& from, const VariableValue& to):
      EvalOp(OperationType::copy, to.idx(), from.idx(), 0, from.lhs()) 
    {
      assert(to.idx()>=0 && from.idx()>=0); 
      assert(to.lhs());
      assert(!from.lhs() || from.idx()!=to.idx());
    }
  };

}

void Minsky::addCopies(const map<int, vector<EvalOp> >& extraCopies, 
                 int idx)
{
  if (extraCopies.count(idx)==0) return;
  const vector<EvalOp>& copiesToAdd=extraCopies.find(idx)->second;
  for (size_t i=0; i<copiesToAdd.size(); ++i)
    {
      equations.push_back(copiesToAdd[i]);
      assert(idx != copiesToAdd[i].out);
      addCopies(extraCopies, copiesToAdd[i].out);
    }
}

// obtain a reference to the variable from which this port obtains its value
const VariableValue& getInputFromVar
(const map<int,VariableValue>& inputFrom, int port, OperationType::Type op)
{
  map<int,VariableValue>::const_iterator p=inputFrom.find(port);
  if (p!=inputFrom.end())
    return p->second;
  else
    // we need to allow for unary operators by creating a temporary
    // group identity variable
    switch (op)
      {
      case OperationType::add: 
      case OperationType::subtract:
        return VariableValue(VariableBase::tempFlow, 0).allocValue();
      case OperationType::multiply: 
      case OperationType::divide:
        return VariableValue(VariableBase::tempFlow, 1).allocValue();
      default:
        throw error("No input for port %d",port);
      }
}
   
// update the inputFrom map, allowing for multi-input binary operators
void Minsky::recordInputFrom
(map<int,VariableValue>& inputFrom, int port, const VariableValue& v, 
 const map<int,int>& operationIdFromInputsPort)
{
  // if there is already a wire going there, insert appropriate op
  if (inputFrom.count(port)) 
    {
      OpAttributes::Type insert_type;
      map<int,int>::const_iterator nextOpId = 
        operationIdFromInputsPort.find(port);
      if (nextOpId==operationIdFromInputsPort.end())
        throw error("Too many inputs"); // to many inputs to a non-operator

      switch (operations[nextOpId->second]->type()) 
        {
        case OperationType::add:
        case OperationType::subtract:
          insert_type = OperationType::add;
          break;
        case OperationType::multiply:
        case OperationType::divide:
          insert_type = OperationType::multiply;
          break;
        default: 
          throw error("Too many inputs");
        }

      VariableValue& v1=inputFrom[port];
      // create new temp variable
      VariableValue new_v = VariableValue(VariableBase::tempFlow).allocValue();

      equations.push_back
        (EvalOp(insert_type, new_v.idx(), v1.idx(), v.idx(), v1.lhs(), v.lhs()));
      inputFrom[port]=new_v;
    } 
  else
    inputFrom[port]=v;
}



void Minsky::constructEquations()
{
  garbageCollect();
  equations.clear();
  integrals.clear();

  map<int,int> operationIdFromInputsPort;
  vector<int> sourceOperations;
  for (Operations::const_iterator o=operations.begin(); 
       o!=operations.end(); ++o)
    {
      for (int p=0; p<o->second->numPorts(); ++p)
        operationIdFromInputsPort[o->second->ports()[p]]=o->first;
      if (o->second->numPorts()==1 || o->second->type()==OperationType::integrate)
        sourceOperations.push_back(o->first);
    }

  // work out the operation order
  OperationOrderer operationOrder;
  for (PortManager::Wires::const_iterator w=wires.begin(); 
       w!=wires.end(); ++w)
    {
      map<int,int>::iterator from = operationIdFromInputsPort.find(w->second.from);
      map<int,int>::iterator to = operationIdFromInputsPort.find(w->second.to);
      if (from!=operationIdFromInputsPort.end() && to!=operationIdFromInputsPort.end()
          // break potential cycles, but not links between integration operations
          && (operations[from->second]->type()!=OperationType::integrate
              || operations[to->second]->type()==OperationType::integrate))
        operationOrder.links[from->second].push_back(to->second);
    }

  // connect up any intermediate variables
  VariableManager::VariableValues::const_iterator val=variables.values.begin();
  for (; val!=variables.values.end(); ++val)
    {
      int inWire=variables.wireToVariable(val->first);
      if (inWire>-1)
        {
          array<int> outWires=variables.wiresFromVariable(val->first);
          if (outWires.size()>0)
            {
              map<int,int>::iterator from = 
                operationIdFromInputsPort.find(wires[inWire].from);
              for (size_t j=0; j<outWires.size(); ++j)
                {
                  map<int,int>::iterator to = 
                    operationIdFromInputsPort.find(wires[outWires[j]].to);
                  if (from!=operationIdFromInputsPort.end() && 
                      to!=operationIdFromInputsPort.end() &&
                      operations[from->second]->type()!=OperationType::integrate)
                    operationOrder.links[from->second].push_back(to->second);
                }
            }
        }
    }
                

  // start with "source" variables, that do not have their inputs wired
  for (VariableManager::iterator v=variables.begin(); v!=variables.end(); ++v)
    if (!variables.InputWired(v->second->name) || 
        v->second->type()==VariableType::integral)
      {
        array<int> attachedWires = WiresAttachedToPort(v->second->outPort());
        for (int w=0; w<attachedWires.size(); ++w)
          {
            map<int,int>::iterator i=operationIdFromInputsPort.
              find(wires[attachedWires[w]].to);
            if (i!=operationIdFromInputsPort.end())
              operationOrder.order(i->second, 1);
          }
      }

  // now add in the source operations
  for (size_t i=0; i<sourceOperations.size(); ++i)
    operationOrder.order(sourceOperations[i], 1);

  vector<pair<int,int> > orderedOperations
    (operationOrder.opOrder.begin(), operationOrder.opOrder.end());
  sort(orderedOperations.begin(), orderedOperations.end(), OrderSecond());

  assert(orderedOperations.size() <= operations.size());
  if (orderedOperations.size() < operations.size())
    throw error("not all operations are wired");

  // A map that maps an input port to variable location that it receives data from
  map<int,VariableValue> inputFrom;
  // map of extra copy operations to be inserted into the operation
  // stream whenever variable given by the index is updated
  map<int, vector<EvalOp> > extraCopies;

  // check whether any wires directly link a variable to another
  // variable, and add copy operations for them. If the variable is an
  // RHS variable, inser the copy operations at the start, otherwise
  // note them in the extraCopies map. Also fill out inputFrom table
  for (PortManager::Wires::const_iterator w=wires.begin(); w!=wires.end(); ++w)
    {
      const VariableValue& rhs=
        variables.getVariableValueFromPort(w->second.from);
      if (rhs.type()!=VariableBase::undefined)
        {
          recordInputFrom
            (inputFrom, w->second.to, rhs, operationIdFromInputsPort);

          const VariableValue& lhs=
            variables.getVariableValueFromPort(w->second.to);

          if (lhs.type()!=VariableBase::undefined &&
              (rhs.type()!=lhs.type() || rhs.idx() != lhs.idx()))
            {
              Copy copy(rhs, lhs);
              if (rhs.lhs())
                // save copy for later
                extraCopies[rhs.idx()].push_back(copy);
              else
                // ensure copy is inserted at beginning
                extraCopies[-1].push_back(copy);
            }
        }
    }

  // prepopulate the integrals vector and add to inputFrom map, as
  // integrals will often be evaluated towards the end of the chain,
  // but feed in (via their stock variables) to earlier on operations.
  for (int i=0; i<orderedOperations.size(); ++i)
    if (IntOp* integ=
        dynamic_cast<IntOp*>(operations[orderedOperations[i].first].get()))
      {
        integrals.push_back(Integral());
        integrals.back().stock=variables.getVariableValue(integ->description());
      }

  // now add the initial set of copy operations
  addCopies(extraCopies, -1);

  vector<Integral>::iterator integral=integrals.begin();

  // copy the operations, in order, to the equation vector
  for (int i=0; i<orderedOperations.size(); ++i)
    {
      OperationPtr& op = operations[orderedOperations[i].first];
      assert(op->numPorts()>0);

      EvalOp e(op->type(), -1);
      e.state=op;

      array<int> outgoingWires = WiresAttachedToPort(op->ports()[0]);
        
      VariableValue v;

      if (op->type()==OperationType::integrate)
        {
          assert(integral!=integrals.end());
          integral->input=inputFrom[op->ports()[1]];
          v=integral->stock;
          ++integral;
        }
      else
        {
          // if any wire connects to an lhs variable, use that,
          // otherwise create a temporary variable
          for (int w=0; w<outgoingWires.size(); ++w)
            {
              Wire& wire=wires[outgoingWires[w]];
              VariableValue& lhs=variables.getVariableValueFromPort(wire.to);
              if (lhs.type()!=VariableBase::undefined) 
                if (v.type()==VariableBase::undefined)
                  v = lhs;
                else
                  // insert a copy operation from v to lhs
                  extraCopies[v.idx()].push_back(Copy(v, lhs));
            }
          if (v.type()==VariableBase::undefined) // create a temporary
            {
              v = VariableValue(VariableBase::tempFlow).allocValue();
            }
        }

      assert(v.idx()!=-1);
      e.out = v.idx();

      if (op->numPorts()>1)
        {
          const VariableValue& v=getInputFromVar(inputFrom, op->ports()[1], e.op);
          e.in1 = v.idx();
          e.flow1 = v.lhs();
        }

      if (op->numPorts()>2)
        {
          const VariableValue& v=getInputFromVar(inputFrom, op->ports()[2], e.op);
          e.in2 = v.idx();
          e.flow2 = v.lhs();
        }
      
      // integration has already been replaced by a copy
      if (e.op != OperationType::integrate)
        {
          equations.push_back(e);
          addCopies(extraCopies, e.out);
          assert(e.out>=0 && (op->numPorts()<1 || e.in1>=0) && 
                 op->numPorts()<2 || e.in2>=0);

          // record destination in inputFrom map (already done for integrals)
          for (int w=0; w<outgoingWires.size(); ++w) 
            recordInputFrom(inputFrom, wires[outgoingWires[w]].to, v, 
                            operationIdFromInputsPort);
        }

    }

      // attach the plots
  for (Plots::Map::iterator i=plots.plots.begin(); i!=plots.plots.end(); ++i)
    {
      PlotWidget& p=i->second;
      p.yvars.clear(); // clear any old associations
      p.xvars.clear(); 
      p.autoScale();
      for (size_t i=0; i<p.ports.size(); ++i)
        if (inputFrom.count(p.ports[i]))
          p.connectVar(inputFrom[p.ports[i]], i);
    }
}

void Minsky::reset()
{
  constructEquations();
  plots.reset(variables);
  t=0;

  if (stockVars.size()>0)
    ode.reset(new RKdata(this));
}

void Minsky::step()
{
 if (edited) 
    {
      reset();
      edited=false;
    }

  // update flow variable
  for (size_t i=0; i<equations.size(); ++i)
    equations[i].eval(&flowVars[0], &stockVars[0]);

  if (ode)
    {
      gsl_odeiv2_driver_set_nmax(ode->driver, nSteps);
      gsl_odeiv2_driver_apply(ode->driver, &t, numeric_limits<double>::max(), 
                              &stockVars[0]);
    }

  for (Plots::Map::iterator i=plots.plots.begin(); i!=plots.plots.end(); ++i)
    i->second.addPlotPt(t);
}

void Minsky::evalEquations(double result[], const double vars[])
{
  // firstly evaluate the flow variables. Initialise to flowVars so
  // that no input vars are correctly initialised
  vector<double> flow(flowVars);
  for (size_t i=0; i<equations.size(); ++i)
    equations[i].eval(&flow[0], vars);

  // then create the result using the Godley table
  for (size_t i=0; i<stockVars.size(); ++i) result[i]=0;
  godleyEval(result, &flow[0]);
  // integrations are kind of a copy
  for (vector<Integral>::iterator i=integrals.begin(); i<integrals.end(); ++i)
    {
      assert(i->input.idx()>=0);
      result[i->stock.idx()] = i->input.lhs()? flow[i->input.idx()]:
        vars[i->input.idx()];
    }
}

void Minsky::jacobian(Matrix& jac, const double sv[])
{
  // firstly evaluate the flow variables
  vector<double> flow(flowVars.size(), 0);
  for (size_t i=0; i<equations.size(); ++i)
    equations[i].eval(&flow[0], sv);

  // then determine the derivatives with respect to variable j
  for (size_t j=0; j<stockVars.size(); ++j)
    {
      vector<double> ds(stockVars.size()), df(flowVars.size());
      ds[j]=1;
      for (size_t i=0; i<equations.size(); ++i)
        equations[i].deriv(&df[0], &ds[0], sv, &flow[0]);
      vector<double> d(stockVars.size());
      godleyEval(&d[0], &df[0]);
      for (vector<Integral>::iterator i=integrals.begin(); 
           i!=integrals.end(); ++i)
        {
          assert(i->stock.idx()>=0 && i->input.idx()>=0);
          d[i->stock.idx()] = 
            i->input.lhs()? df[i->input.idx()]: ds[i->input.idx()];
        }
      for (size_t i=0; i<stockVars.size(); i++)
        jac(i,j)=d[i];
    }
  
}

void Minsky::Save(const char* filename) 
{
  ofstream of(filename);
  xml_pack_t saveFile(of, schemaURL);
  saveFile.prettyPrint=true;
  garbageCollect();
  xml_pack(saveFile, "Minsky", schema1::Minsky(*this));
}

void Minsky::Load(const char* filename) 
{
  
  clearAll();

  // current schema
  schema1::Minsky currentSchema;
  ifstream inf(filename);
  xml_unpack_t saveFile(inf);
  xml_unpack(saveFile, "Minsky", currentSchema);

  if (currentSchema.version == currentSchema.schemaVersion)
    *this = currentSchema;
  else
    { // fall back to the ill-defined schema '0'
      schema0::Minsky m;
      m.load(filename);
      *this=m;
    }

  variables.makeConsistent();
  for (GodleyItems::iterator g=godleyItems.begin(); g!=godleyItems.end(); ++g)
    g->second.update();

  edited=true;
}

void Minsky::ExportSchema(const char* filename, int schemaLevel)
{
  xsd_generate_t x;
  // currently, there is only 1 schema level, so ignore second arg
  xsd_generate(x,"Minsky",schema1::Minsky());
  ofstream f(filename);
  x.output(f,schemaURL);
}

array<int> Minsky::opOrder()
{
  array<int> r;
  for (size_t i=0; i<equations.size(); ++i)
    if (equations[i].state)
      for (Operations::iterator j=operations.begin(); j!=operations.end(); ++j)
        if (equations[i].state==j->second)
          r<<=j->first;
  return r;
}
