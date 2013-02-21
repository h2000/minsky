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
#include "cairoItems.h"

#include <schema/schema0.h>
#include <schema/schema1.h>

using namespace minsky;
using namespace classdesc;

namespace
{
  const char* schemaURL="http://minsky.sf.net/minsky";

  inline bool isFinite(const double y[], size_t n)
  {
    for (size_t i=0; i<n; ++i)
      if (!finite(y[i])) return false;
    return true;
  }

  /*
    For using GSL Runge-Kutta routines
  */

  int function(double t, const double y[], double f[], void *params)
  {
    if (params==NULL) return GSL_EBADFUNC;
    try
      {
        ((Minsky*)params)->evalEquations(f,y);
      }
    catch (std::exception& e)
      {
        Tcl_AppendResult(interp(),e.what(),NULL);
        Tcl_AppendResult(interp(),"\n",NULL);
        return GSL_EBADFUNC;
      }
    return GSL_SUCCESS;
  }

  int jacobian(double t, const double y[], double * dfdy, double dfdt[], void * params)
  {
    if (params==NULL) return GSL_EBADFUNC;
        Minsky::Matrix jac(ValueVector::stockVars.size(), dfdy);
    try
      {
        ((Minsky*)params)->jacobian(jac,y);
      }
     catch (std::exception& e)
      {
        Tcl_AppendResult(interp(),e.what(),NULL);
        Tcl_AppendResult(interp(),"\n",NULL);
        return GSL_EBADFUNC;
      }   
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
      // TODO: implicit methods are better for stiffer systems, but
      // require the jacobian (which I've taken special care to
      // provide). Make as a user option to select solver order, and
      // whether implicit or explicit methods are used.
//      driver = gsl_odeiv2_driver_alloc_y_new
//        (&sys, gsl_odeiv2_step_rk4imp, minsky->stepMax, minsky->epsAbs, 
//         minsky->epsRel);
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

namespace 
{
  Minsky* l_minsky=NULL;
}

namespace minsky
{
  Minsky& minsky()
  {
    static Minsky s_minsky;
    if (l_minsky)
      return *l_minsky;
    else
      return s_minsky;
  }

  LocalMinsky::LocalMinsky(Minsky& minsky) {l_minsky=&minsky;}
  LocalMinsky::~LocalMinsky() {l_minsky=NULL;}

  // a hook for recording when the minsky model's state changes
  void member_entry_hook(int argc, CONST84 char** argv)
  {
    if (argc>1) minsky().markEdited();
  }

  TCL_obj_t& minskyTCL_obj() 
  {
    static TCL_obj_t t;
    static int dum=(t.member_entry_hook=member_entry_hook,1);
    return t;
  }

  tclvar TCL_obj_lib("ecolab_library",ECOLAB_LIB);
  int TCL_obj_minsky=
    (
     TCL_obj_init(minsky()),
     ::TCL_obj(minskyTCL_obj(),"minsky",minsky()),
     1
     );


  Minsky::Minsky(): reset_needed(true), m_zoomFactor(1),
                    port(ports), wire(wires), op(operations), 
                    constant(operations), integral(operations), var(variables),
                    value(variables.values), plot(plots.plots), 
                    godleyItem(godleyItems), groupItem(groupItems),
                    t(0), stepMin(0), stepMax(0.1), nSteps(1),
                    epsAbs(1e-3), epsRel(1e-2) 
  {
    m_edited=false; // needs to be here, because the GodleyIcon constructor calls markEdited
  }

  void Minsky::clearAllMaps()
  {
    wires.clear(); 
    ports.clear();
    godleyItems.clear();
    operations.clear();
    variables.clear();
    variables.values.clear();
    groupItems.clear();
    plots.clear();
    
    flowVars.clear();
    stockVars.clear();

    reset_needed=true;
  }

  void Minsky::clearAllGetterSetters() 
  {
    // need also to clear the GetterSetterPtr variables, as these
    // potentially hold onto objects
    op.clear();
    constant.clear();
    integral.clear();
    var.clear();
    // need to clear some variable referenced in the godleyItem getter/setter
    // TODO: I'm not happy with this piece of merde.
    godleyItem.flowVars.clear();
    godleyItem.stockVars.clear();
  }

  const char* Minsky::minskyVersion=MINSKY_VERSION;

  int Minsky::addWire(TCL_args args) {
    int from=args, to=args;
    // wire must go from an output port to an input port
    if (ports[from].input || !ports[to].input)
      return -1;

    // check we're not wiring an operator to its input
    // TODO: move this into an operationManager class
    for (Operations::const_iterator o=operations.begin(); 
         o!=operations.end(); ++o)
      if (o->second->selfWire(from, to))
        return -1;

    // check that multiple input wires are only to binary ops.
    if (WiresAttachedToPort(to).size()>=1 && !ports[to].multiWireAllowed())
      return -1;

    // check whether variable manager will allow the connection
    if (!variables.addWire(from, to))
      return -1;

    // check that a wire doesn't already exist connecting these two ports
    for (Wires::const_iterator w=wires.begin(); w!=wires.end(); ++w)
      if (w->second.from==from && w->second.to==to)
        return -1;

    Wire w(from, to);
    w.coords(args);
    if (w.Coords().size()<4)
      return -1;
    int id=PortManager::addWire(w);
    markEdited();
    return id;
  }

  void Minsky::deleteWire(TCL_args args)
  {
    int id=args;
    if (wires.count(id))
      {
        variables.deleteWire(wires[id].to);
        PortManager::deleteWire(id);
        markEdited();
      }
  }


  int Minsky::AddOperation(const char* o)
  {
    OperationPtr newOp(static_cast<OperationType::Type>
                       (enumKey<OperationType::Type>(o)));
    if (!newOp) return -1;
    int id=operations.empty()? 0: operations.rbegin()->first+1;
    operations.insert(make_pair(id, newOp));
    markEdited();
    return id;
  }

  int Minsky::CopyOperation(int id)
  {
    Operations::iterator source=operations.find(id);
    if (source==operations.end()) return -1;
    int newId=operations.empty()? 0: operations.rbegin()->first+1;
    OperationPtr newOp = source->second->clone();
    operations.insert(make_pair(newId, newOp));
    markEdited();
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
        // ticket #199, remove references held by getter/setter
        this->op.clear();
        integral.clear();
        constant.clear();
        markEdited();
      }
  }

  int Minsky::Group(float x0, float y0, float x1, float y1)
  {
    int id=groupItems.empty()? 0: groupItems.rbegin()->first+1;
    GroupIcon& g=groupItems.insert(make_pair(id, GroupIcon(id))).first->second;
    g.createGroup(x0,y0,x1,y1);
    if (g.empty())
      {
        groupItems.erase(id);
        return -1;
      }
    markEdited();
    return id;
  }

  void Minsky::Ungroup(int id)
  {
    groupItems[id].ungroup();
    groupItems.erase(id);
    markEdited();
  }

  int Minsky::CopyGroup(int id)
  {
    GroupIcons::iterator srcIt=groupItems.find(id);
    if (srcIt==groupItems.end()) return -1; //src not found
    int newId=groupItems.rbegin()->first+1;
    GroupIcon& g=
      groupItems.insert(make_pair(newId, GroupIcon(newId))).first->second;
    g.copy(srcIt->second);
    markEdited();
    return newId;
  }

  int Minsky::InsertGroupFromFile(const char* file)
  {
    schema1::Minsky currentSchema;
    ifstream inf(file);
    xml_unpack_t saveFile(inf);
    xml_unpack(saveFile, "Minsky", currentSchema);

    if (currentSchema.version != currentSchema.schemaVersion)
      throw error("Invalid Minsky schema file");

    int newId=groupItems.rbegin()->first+1;
    GroupIcon& g=
      groupItems.insert(make_pair(newId, GroupIcon(newId))).first->second;
    currentSchema.populateGroup(g);
    return newId;
  }

  array<int> Minsky::unwiredOperations() const
  {
    array<int> ret;
    for (Operations::const_iterator op=operations.begin(); op!=operations.end(); ++op)
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
        int id=variables.addVariable(v);
        markEdited();
        return id;
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
      else
        ++v;

    variables.makeConsistent();

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
    struct Copy: public EvalOpPtr
    {
      Copy(const VariableValue& from, const VariableValue& to):
        EvalOpPtr(OperationType::copy, to.idx(), from.idx(), 0, from.lhs()) 
      {
        assert(to.idx()>=0 && from.idx()>=0); 
        assert(to.lhs());
        assert(!from.lhs() || from.idx()!=to.idx());
      }
    };

  }

  void Minsky::addCopies(const map<int, vector<EvalOpPtr> >& extraCopies, 
                         int idx)
  {
    if (extraCopies.count(idx)==0) return;
    const vector<EvalOpPtr>& copiesToAdd=extraCopies.find(idx)->second;
    for (size_t i=0; i<copiesToAdd.size(); ++i)
      {
        equations.push_back(copiesToAdd[i]);
        assert(idx != copiesToAdd[i]->out);
        addCopies(extraCopies, copiesToAdd[i]->out);
      }
  }

  // obtain a reference to the variable from which this port obtains its value
  const VariableValue& getInputFromVar
  //  (const map<int,VariableValue>& inputFrom, int port, OperationType::Type op)
  (const map<int,VariableValue>& inputFrom, int port, const OperationBase& op)
  {
    map<int,VariableValue>::const_iterator p=inputFrom.find(port);
    if (p!=inputFrom.end())
      return p->second;
    else
      // we need to allow for unary operators by creating a temporary
      // group identity variable
      switch (op.type())
        {
        case OperationType::add: 
        case OperationType::subtract:
          return VariableValue(VariableBase::tempFlow, 0).allocValue();
        case OperationType::multiply: 
        case OperationType::divide:
          return VariableValue(VariableBase::tempFlow, 1).allocValue();
        default:
          Minsky::displayErrorItem(op.x(), op.y());
          throw error("No input for port %d",port);
        }
  }
   
  // update the inputFrom map, allowing for multi-input binary operators
  void Minsky::recordInputFrom
  (map<int,VariableValue>& inputFrom, int port, const VariableValue& v, 
   const map<int,int>& operationIdFromInputsPort, multimap<int, EvalOpPtr>& extraOps)
  {
    // if there is already a wire going there, insert appropriate op
    if (inputFrom.count(port)) 
      {
        OpAttributes::Type insert_type;
        map<int,int>::const_iterator nextOpId = 
          operationIdFromInputsPort.find(port);
        if (nextOpId==operationIdFromInputsPort.end())
          {
            // can only possibly be a variable - if any other, we're
            // severely screwed, and this will just indicate the
            // origin of the canvas.
            const VariablePtr v=variables.getVariableFromPort(port);
            displayErrorItem(v->x(), v->y());
            throw error("Too many inputs"); // to many inputs to a non-operator
          }

        OperationBase& nextOp=*operations[nextOpId->second];
        switch (nextOp.type()) 
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
            {
              displayErrorItem(nextOp.x(), nextOp.y());
              throw error("Too many inputs");
            }
          }

        VariableValue& v1=inputFrom[port];
        // create new temp variable
        VariableValue new_v = VariableValue(VariableBase::tempFlow).allocValue();

        extraOps.insert
          (make_pair
           (nextOpId->second, 
            EvalOpPtr
            (insert_type, new_v.idx(), v1.idx(), v.idx(), v1.lhs(), v.lhs())));
        inputFrom[port]=new_v;
      } 
    else
      inputFrom[port]=v;
  }

  namespace 
  {
    /*
      follow links from a variable, through a chain of variables until
      landing on an operation, in which case connect the source to the
      target, or if landing on something else, ignore
    */
    void connectVariableChains(const string& name, 
                               const map<int,int>& operationIdFromInputsPort,
                               map<int,int>::const_iterator from,
                               Minsky& minsky,
                               OperationOrderer& operationOrder)
    {
      array<int> outWires=minsky.variables.wiresFromVariable(name);
      for (size_t j=0; j<outWires.size(); ++j)
        {
          // check if wired to a variable, and recursively call if it is
          VariablePtr v=minsky.variables.
            getVariableFromPort(minsky.wires[outWires[j]].to);
          if (v->type()!=VariableType::undefined)
            connectVariableChains(v->Name(), operationIdFromInputsPort, from,
                                  minsky, operationOrder);
          else
            {
              map<int,int>::const_iterator to = 
                operationIdFromInputsPort.find
                (minsky.wires[outWires[j]].to);
              
              if (from!=operationIdFromInputsPort.end() && 
                  to!=operationIdFromInputsPort.end() &&
                  minsky.operations[from->second]->type()!=OperationType::integrate)
                operationOrder.links[from->second].push_back(to->second);
            }
        }
    }
    
  }
  

  void Minsky::constructEquations()
  {
    if (cycleCheck()) throw error("cyclic network detected");
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
          connectVariableChains
            (val->first,  operationIdFromInputsPort, 
             operationIdFromInputsPort.find(wires[inWire].from), *this,
             operationOrder);
      }
    // start with "source" variables, that do not have their inputs wired
    for (VariableManager::iterator v=variables.begin(); v!=variables.end(); ++v)
      if (!variables.InputWired(v->second->Name()) || 
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
      {
        // find offending operations
        set<int> orderedOps;
        for (size_t i=0; i<orderedOperations.size(); ++i)
          orderedOps.insert(orderedOperations[i].first);
        for (Operations::const_iterator op=operations.begin(); 
             op!=operations.end(); ++op)
          if (!orderedOps.count(op->first))
            displayErrorItem(op->second->x(), op->second->y());
        throw error("not all operations are wired");
      }

    // A map that maps an input port to variable location that it receives data from
    map<int,VariableValue> inputFrom;
    // map of extra copy operations to be inserted into the operation
    // stream whenever variable given by the index is updated
    map<int, vector<EvalOpPtr> > extraCopies;
    // extra binary ops for multi-input port support
    multimap<int, EvalOpPtr> extraOps;

    // check whether any wires directly link a variable to another
    // variable, and add copy operations for them. If the variable is an
    // RHS variable, insert the copy operations at the start, otherwise
    // note them in the extraCopies map. Also fill out inputFrom table
    for (PortManager::Wires::const_iterator w=wires.begin(); w!=wires.end(); ++w)
      {
        const VariableValue& rhs=
          variables.getVariableValueFromPort(w->second.from);
        if (rhs.type()!=VariableBase::undefined)
          {
            recordInputFrom
              (inputFrom, w->second.to, rhs, operationIdFromInputsPort, extraOps);

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
          integrals.back().operation=integ;
        }

    // now add the initial set of copy operations
    addCopies(extraCopies, -1);

    vector<Integral>::iterator integral=integrals.begin();

    // copy the operations, in order, to the equation vector
    for (int i=0; i<orderedOperations.size(); ++i)
      {
        // insert any extra ops that need to be added before this one
        pair<multimap<int, EvalOpPtr>::const_iterator, 
          multimap<int, EvalOpPtr>::const_iterator> extraOpsToBeInserted=
          extraOps.equal_range(orderedOperations[i].first);
        for (multimap<int, EvalOpPtr>::const_iterator e=
               extraOpsToBeInserted.first; e!=extraOpsToBeInserted.second; ++e)
          equations.push_back(e->second);

        OperationPtr& op = operations[orderedOperations[i].first];
        assert(op->numPorts()>0);

        EvalOpPtr e(op->type(), -1);
        e->state=op;

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
        e->out = v.idx();

        if (op->numPorts()>1)
          {
            const VariableValue& v=getInputFromVar(inputFrom, op->ports()[1], *op);
            e->in1 = v.idx();
            e->flow1 = v.lhs();
          }

        if (op->numPorts()>2)
          {
            const VariableValue& v=getInputFromVar(inputFrom, op->ports()[2], *op);
            e->in2 = v.idx();
            e->flow2 = v.lhs();
          }
      
        // integration has already been replaced by a copy
        if (e->type() != OperationType::integrate)
          {
            equations.push_back(e);
            addCopies(extraCopies, e->out);
            assert(e->out>=0 && (op->numPorts()<1 || e->in1>=0) && 
                   op->numPorts()<2 || e->in2>=0);

            // record destination in inputFrom map (already done for integrals)
            for (int w=0; w<outgoingWires.size(); ++w) 
              recordInputFrom(inputFrom, wires[outgoingWires[w]].to, v, 
                              operationIdFromInputsPort, extraOps);
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
    for (EvalOpVector::iterator e=equations.begin(); e!=equations.end(); ++e)
      (*e)->reset();
  }

  void Minsky::reset()
  {
    constructEquations();
    // if no stock variables in system, add a dummy stock variable to
    // make the simulation proceed
    if (stockVars.empty()) stockVars.resize(1,0);

    plots.reset(variables);
    t=0;

    if (stockVars.size()>0)
      ode.reset(new RKdata(this));
  }

  void Minsky::step()
  {
    if (reset_needed) 
      {
        reset();
        reset_needed=false;
        // update flow variable
        for (size_t i=0; i<equations.size(); ++i)
          equations[i]->eval(&flowVars[0], &stockVars[0]);
      }


    if (ode)
      {
        gsl_odeiv2_driver_set_nmax(ode->driver, nSteps);
        int err=gsl_odeiv2_driver_apply(ode->driver, &t, numeric_limits<double>::max(), 
                                &stockVars[0]);
        switch (err)
          {
          case GSL_SUCCESS: case GSL_EMAXITER: break;
//          case GSL_ENOPROG: 
//            throw error("Simulation failing to progress, try to reduce minimum step size");
          case GSL_FAILURE:
            throw error("unspecified error GSL_FAILURE returned");
          case GSL_EBADFUNC: 
            gsl_odeiv2_driver_reset(ode->driver);
            throw error("Invalid arithmetic operation detected");
          default:
            throw error("gsl error: %s",gsl_strerror(err));
          }
      }

    // update flow variables
    for (size_t i=0; i<equations.size(); ++i)
      equations[i]->eval(&flowVars[0], &stockVars[0]);

    for (Plots::Map::iterator i=plots.plots.begin(); i!=plots.plots.end(); ++i)
      i->second.addPlotPt(t);
  }

  string Minsky::diagnoseNonFinite() const
  {
    // firstly check if any variables are not finite
    for (VariableManager::VariableValues::const_iterator v=variables.values.begin();
         v!=variables.values.end(); ++v)
      if (!finite(v->second.value()))
        return v->first;

    // now check operator equations
    for (EvalOpVector::const_iterator e=equations.begin(); e!=equations.end(); ++e)
      if (!finite(flowVars[(*e)->out]))
        return OperationType::typeName((*e)->type());
    return "";
  }

  void Minsky::evalEquations(double result[], const double vars[])
  {
    // firstly evaluate the flow variables. Initialise to flowVars so
    // that no input vars are correctly initialised
    vector<double> flow(flowVars);
    for (size_t i=0; i<equations.size(); ++i)
      equations[i]->eval(&flow[0], vars);

    // then create the result using the Godley table
    for (size_t i=0; i<stockVars.size(); ++i) result[i]=0;
    godleyEval(result, &flow[0]);
    // integrations are kind of a copy
    for (vector<Integral>::iterator i=integrals.begin(); i<integrals.end(); ++i)
      {
        if (i->input.idx()<0)
          {
            if (i->operation)
              displayErrorItem(i->operation->x(), i->operation->y());
            throw error("integral not wired");
          }
        result[i->stock.idx()] = i->input.lhs()? flow[i->input.idx()]:
          vars[i->input.idx()];
      }
  }

  void Minsky::jacobian(Matrix& jac, const double sv[])
  {
    // firstly evaluate the flow variables. Initialise to flowVars so
    // that no input vars are correctly initialised
    vector<double> flow=flowVars;
    for (size_t i=0; i<equations.size(); ++i)
      equations[i]->eval(&flow[0], sv);

    // then determine the derivatives with respect to variable j
    for (size_t j=0; j<stockVars.size(); ++j)
      {
        vector<double> ds(stockVars.size()), df(flowVars.size());
        ds[j]=1;
        for (size_t i=0; i<equations.size(); ++i)
          equations[i]->deriv(&df[0], &ds[0], sv, &flow[0]);
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
    m_edited=false;
  }

namespace
{
  // comparison operation used for removing duplicate wires
  struct LessWire
  {
    bool operator()(const Minsky::Wires::value_type& x, 
                   const Minsky::Wires::value_type& y) const
    {
      return x.second.from<y.second.from || 
        x.second.from==y.second.from && x.second.to<y.second.to;
    }
  };
}

  void Minsky::Load(const char* filename) 
  {
  
    clearAllMaps();
    clearAllGetterSetters();

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

    for (GroupIcons::iterator g=groupItems.begin(); g!=groupItems.end(); ++g)
      {
        // ensure group attributes correctly set
        const vector<int>& vars= g->second.variables();
        for (vector<int>::const_iterator i=vars.begin(); i!=vars.end(); ++i)
          {
            VariablePtr& v=variables[*i];
            v->group=g->first;
            v->visible=g->second.displayContents();
          }
        const vector<int>& ops= g->second.operations();
        for (vector<int>::const_iterator i=ops.begin(); i!=ops.end(); ++i)
          {
            OperationPtr& o=operations[*i];
            o->group=g->first;
            o->visible=g->second.displayContents();
          }
        const vector<int>& gwires= g->second.wires();
        for (vector<int>::const_iterator i=gwires.begin(); i!=gwires.end(); ++i)
          {
            Wire& w=wires[*i];
            w.group=g->first;
            w.visible=g->second.displayContents();
          }
      }

    // remove multiply connected wires (ticket 171)
    set<Wires::value_type, LessWire> wireSet(wires.begin(), wires.end());
    wires.clear();
    wires.insert(wireSet.begin(), wireSet.end());

    m_edited=false;
    reset_needed=true;
  }

  void Minsky::ExportSchema(const char* filename, int schemaLevel)
  {
    xsd_generate_t x;
    // currently, there is only 1 schema level, so ignore second arg
    xsd_generate(x,"Minsky",schema1::Minsky());
    ofstream f(filename);
    x.output(f,schemaURL);
  }

  int Minsky::opIdOfEvalOp(const EvalOpBase& e) const
  {
    if (e.state)
      for (Operations::const_iterator j=operations.begin(); 
           j!=operations.end(); ++j)
        if (e.state==j->second)
          return j->first;
    return -1;
  }


  array<int> Minsky::opOrder() const
  {
    array<int> r;
    for (size_t i=0; i<equations.size(); ++i)
      r<<opIdOfEvalOp(*equations[i]);
    return r;
  }

  void Minsky::Zoom(float xOrigin, float yOrigin, float factor)
  {
    for (Wires::iterator w=wires.begin(); w!=wires.end(); ++w)
      w->second.zoom(xOrigin, yOrigin, factor);
    for (Operations::iterator o=operations.begin(); o!=operations.end(); ++o)
      o->second->zoom(xOrigin, yOrigin, factor);
    for (VariableManager::iterator v=variables.begin(); v!=variables.end(); ++v)
      v->second->zoom(xOrigin, yOrigin, factor);
    for (GodleyItems::iterator g=godleyItems.begin(); g!=godleyItems.end(); ++g)
      g->second.zoom(xOrigin, yOrigin, factor);
    for (GroupIcons::iterator g=groupItems.begin(); g!=groupItems.end(); ++g)
      g->second.zoom(xOrigin, yOrigin, factor);
    for (Plots::Map::iterator p=plots.plots.begin(); p!=plots.plots.end(); ++p)
      p->second.zoom(xOrigin, yOrigin, factor);
    m_zoomFactor*=factor;
  }

  void Minsky::setZoom(float factor)
  {
    for (Operations::iterator o=operations.begin(); o!=operations.end(); ++o)
      if (o->second->group==-1)
        o->second->setZoom(factor);
    for (VariableManager::iterator v=variables.begin(); v!=variables.end(); ++v)
      if (v->second->group==-1)
        v->second->setZoom(factor);
    for (GroupIcons::iterator g=groupItems.begin(); g!=groupItems.end(); ++g)
      if (g->second.group()==-1)
        g->second.setZoom(factor);
    for (GodleyItems::iterator g=godleyItems.begin(); g!=godleyItems.end(); ++g)
      g->second.zoomFactor=factor;
    for (Plots::Map::iterator p=plots.plots.begin(); p!=plots.plots.end(); ++p)
      p->second.zoomFactor=factor;
    m_zoomFactor=factor;
  }

  void Minsky::AddVariableToGroup(int groupId, int varId)
  {
    GroupIcons::iterator g=groupItems.find(groupId);
    VariableManager::iterator v=variables.find(varId);
    if (g!=groupItems.end() && v!=variables.end())
      {
        if (v->second->group!=-1)
          {
            GroupIcons::iterator pg=groupItems.find(v->second->group);
            if (pg!=groupItems.end())
              pg->second.removeVariable(*v);
          }
        g->second.addVariable(*v);
        g->second.addAnyWires(v->second->ports());
      }
  }

  void Minsky::RemoveVariableFromGroup(int groupId, int varId)
  {
    GroupIcons::iterator g=groupItems.find(groupId);
    VariableManager::iterator v=variables.find(varId);
    if (g!=groupItems.end() && v!=variables.end() && v->second->group==groupId)
      {
        g->second.removeVariable(*v);
        g->second.removeAnyWires(v->second->ports());
        if (g->second.parent()!=-1)
          {
            GroupIcons::iterator pg=groupItems.find(g->second.parent());
            if (pg!=groupItems.end())
              pg->second.addVariable(*v);
          }
      }
  }

  void Minsky::AddOperationToGroup(int groupId, int opId)
  {
    GroupIcons::iterator g=groupItems.find(groupId);
    Operations::iterator o=operations.find(opId);
    if (g!=groupItems.end() && o!=operations.end() && o->second->group!=groupId)
      {
        if (o->second->group!=-1)
          {
            GroupIcons::iterator pg=groupItems.find(o->second->group);
            if (pg!=groupItems.end())
              pg->second.removeOperation(*o);
          }
        g->second.addOperation(*o);
        g->second.addAnyWires(o->second->ports());
      }
  }

  void Minsky::RemoveOperationFromGroup(int groupId, int opId)
  {
    GroupIcons::iterator g=groupItems.find(groupId);
    Operations::iterator o=operations.find(opId);
    if (g!=groupItems.end() && o!=operations.end() && o->second->group==groupId)
      {
        g->second.removeOperation(*o);
        g->second.removeAnyWires(o->second->ports());
        if (g->second.parent()!=-1)
          {
            GroupIcons::iterator pg=groupItems.find(g->second.parent());
            if (pg!=groupItems.end())
              pg->second.addOperation(*o);
          }
      }
  }

  bool Minsky::AddGroupToGroup(int destGroup, int groupId)
  {
    GroupIcons::iterator g=groupItems.find(groupId);
    GroupIcons::iterator dg=groupItems.find(destGroup);
    if (g!=groupItems.end() && dg!=groupItems.end())
      {
        if (g->second.parent()!=-1)
          {
            GroupIcons::iterator pg=groupItems.find(g->second.parent());
            if (pg!=groupItems.end())
              pg->second.removeGroup(*g);
          }
        return dg->second.addGroup(*g);
      }
    return false;
  }

  void Minsky::RemoveGroupFromGroup(int destGroup, int groupId)
  {
    GroupIcons::iterator g=groupItems.find(groupId);
    GroupIcons::iterator dg=groupItems.find(destGroup);
    if (g!=groupItems.end() && dg!=groupItems.end())
      {
        dg->second.removeGroup(*g);
        if (dg->second.parent()!=-1)
          {
            GroupIcons::iterator pg=groupItems.find(dg->second.parent());
            if (pg!=groupItems.end())
              pg->second.addGroup(*g);
          }
      }
  }

  namespace
  {
    struct Network: public multimap<int,int>
    {
      set<int> portsVisited;
      vector<int> stack;
      // depth-first network walk, return true if cycle detected
      bool followWire(int p)
      {
        if (!portsVisited.insert(p).second)
          { //traverse finished, check for cycle along branch
            if (::find(stack.begin(), stack.end(), p) != stack.end())
              {
                const Port& port=minsky().ports[p];
                Minsky::displayErrorItem(port.x(), port.y());
                return true;
              }
            else
              return false;
          }
        stack.push_back(p);
        pair<iterator,iterator> range=equal_range(p);
        for (iterator i=range.first; i!=range.second; ++i)
          if (followWire(i->second))
            return true;
        stack.pop_back();
        return false;
      }
    };
  }
    
  bool Minsky::cycleCheck() const
  {
    // construct the network schematic
    Network net;
    for (Wires::const_iterator w=wires.begin(); w!=wires.end(); ++w)
      net.insert(make_pair(w->second.from, w->second.to));
    for (Operations::const_iterator o=operations.begin(); 
         o!=operations.end(); ++o)
      for (int j=1; j<o->second->numPorts(); ++j)
        if (o->second->type()!=OperationType::integrate)
          net.insert(make_pair(o->second->ports()[j], o->second->ports()[0]));
    for (VariableManager::const_iterator v=variables.begin(); 
         v!=variables.end(); ++v)
      if (v->second->numPorts()>1)
        net.insert(make_pair(v->second->inPort(), v->second->outPort()));

    for (Ports::const_iterator p=ports.begin(); p!=ports.end(); ++p)
      if (!p->second.input && !net.portsVisited.count(p->first))
        if (net.followWire(p->first))
          return true;
    return false;
  }

  bool Minsky::checkEquationOrder() const
  {
    array<bool> fvInit(flowVars.size(), false);
    // firstly, find all flowVars that are constants
    for (VariableManager::VariableValues::const_iterator v=
           variables.values.begin(); v!=variables.values.end(); ++v)
      if (!variables.InputWired(v->first) && v->second.idx()>=0)
        fvInit[v->second.idx()]=true;

    for (EvalOpVector::const_iterator e=equations.begin(); 
         e!=equations.end(); ++e)
      {
        const EvalOpBase& eo=**e;
        if (eo.out < 0|| eo.numArgs()>0 && eo.in1<0 ||
            eo.numArgs() > 1 && eo.in2<0)
          {
            cerr << "Incorrectly wired operation "<<opIdOfEvalOp(eo)<<endl;
            return false;
          }
        switch  (eo.numArgs())
          {
          case 0:
            fvInit[eo.out]=true;
            break;
          case 1:
            fvInit[eo.out]=!eo.flow1 || fvInit[eo.in1];
            break;
          case 2:
            // we need to check if an associated binary operator has
            // an unwired input, and if so, treat its input as
            // initialised, since it has already been initialised in
            // getInputFromVar()
            Operations::const_iterator op=operations.find(opIdOfEvalOp(eo));
            if (op!=operations.end())
              switch (op->second->type())
                {
                case OperationType::add: case OperationType::subtract:
                case OperationType::multiply: case OperationType::divide:
                  fvInit[eo.in1] |= 
                    WiresAttachedToPort(op->second->ports()[1]).size()==0;
                  fvInit[eo.in2] |= 
                    WiresAttachedToPort(op->second->ports()[3]).size()==0;
                }
            
            fvInit[eo.out]=
              (!eo.flow1 ||  fvInit[eo.in1]) && (!eo.flow2 ||  fvInit[eo.in2]);
            break;
          }
        if (!fvInit[eo.out])
          cerr << "Operation "<<opIdOfEvalOp(eo)<<" out of order"<<endl;
      }
    
    return all(fvInit);
  }


  namespace
  {
    struct OperationIcon: public ecolab::cairo::CairoImage
    {
      OperationPtr op;
      OperationIcon(const char* imageName, const char* opName):
        CairoImage(imageName), 
        op(OperationType::Type(enumKey<OperationType::Type>(opName))) {}
      void draw()
      {
        //xScale=yScale=2;
        initMatrix();
        cairo_select_font_face(cairoSurface->cairo(), "sans-serif", 
                  CAIRO_FONT_SLANT_ITALIC,CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cairoSurface->cairo(),12);
        cairo_set_line_width(cairoSurface->cairo(),1.5);
        RenderOperation(op, cairoSurface->cairo()).draw();
        cairoSurface->blit();
      }
    };
  }

  void Minsky::operationIcon(TCL_args args) const
  {
    OperationIcon(args[0], args[1]).draw();
  }

  void Minsky::displayErrorItem(float x, float y)
  {
    tclcmd() << "catch {indicateCanvasItemInError"<<x<<y<<"}\n";
    Tcl_ResetResult(interp());
  }
}

