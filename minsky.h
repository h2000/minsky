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

#ifndef MINSKY_H
#define MINSKY_H

#include <vector>
#include <string>
#include <set>
using namespace std;

#include <ecolab.h>
#include <xml_pack_base.h>
#include <xml_unpack_base.h>
using namespace ecolab;
using namespace classdesc;

#include "godleyIcon.h"
#include "operation.h"
#include "wire.h"
#include "portManager.h"
#include "plotWidget.h"
#include "groupIcon.h"
#include "version.h"
#include "variable.h"
#include "equations.h"

namespace minsky
{
  // An integral is an additional stock variable, that integrates its flow variable
  struct Integral
  {
    VariableValue stock;
    VariableValue input;
    Integral(VariableValue input=VariableValue()): 
      stock(VariableBase::integral), input(input) {}
  };

  struct RKdata; // an internal structure for holding Runge-Kutta data

  // a place to put working variables of the Minsky class that needn't
  // be serialised.
  struct MinskyExclude
  {
    vector<EvalOp> equations;
    vector<Integral> integrals;
    shared_ptr<RKdata> ode;
  };

  /// convenience class for accessing matrix elements from a data array
  class MinskyMatrix
  {
    size_t n;
    double *data;
  public:
    MinskyMatrix(size_t n, double* data): n(n), data(data) {}
    double& operator()(size_t i, size_t j) {return data[i*n+j];}
    double operator()(size_t i, size_t j) const {return data[i*n+j];}
  };
  /**
     convenience class for accessing elements of a map from TCL
  */
  template <class K, class T>
  class GetterSetter: public T
  {
    std::map<K, T>& map;
  public:
    K key; ///<last key gotten
    void get(TCL_args args) {
      TCL_args tmp(args);
      tmp>>key;
      if (map.count(key)) {T::operator=(map[key]);}
      else
        throw error("%s not found",(char*)args[0]);
    }
    void set(TCL_args args) {
      if (args.count) args>>key;
      map[key]=*this;
    }
    GetterSetter(std::map<K,T>& m): map(m) {}
    // asignment is do nothing, as reference member is created as part
    // of constructor
    void operator=(const GetterSetter&) {}
  };

  template <class K, class T>
  class GetterSetterPtr
  {
    std::map<K, T>& map;
  public:
    // nb, in spite of appearances, this approach does not work well
    // with non-shared_pointer value types
    void get(TCL_args args) {
      string cmdPrefix=args[-1];
      K key;
      TCL_args tmp(args);
      tmp>>key;
      typename std::map<K, T>::iterator i=map.find(key);
      if (i!=map.end()) 
        {
          cmdPrefix.erase(cmdPrefix.rfind(".get"));
          // register current object with TCL
          TCL_obj(null_TCL_obj, cmdPrefix, i->second);
        }
      else
        throw error("object not found: %s[%s]",(char*)args[-1],(char*)args[0]);
    }
    // for backward compatibility
    void set(TCL_args args) {}
    GetterSetterPtr(std::map<K,T>& m): map(m) {}
    // asignment is do nothing, as reference member is created as part
    // of constructor
    void operator=(const GetterSetterPtr&) {}
  };


  class Minsky: public ValueVector, public MinskyExclude, public PortManager
  {
    CLASSDESC_ACCESS(Minsky);
    bool edited;

    /// add the extra copy operations performed when variableValue idx
    /// is updated.  Use idx=-1 for the initial set
    void addCopies(const map<int, vector<EvalOp> >& extraCopies, 
                   int idx);

    /// update the inputFrom map, allowing for multi-input binary
    /// operators. Used in constructEquations
    void recordInputFrom
    (map<int,VariableValue>& inputFrom, int port, const VariableValue& v, 
     const map<int,int>& operationIdFromInputsPort);

  public:

    GodleyTable godley; // deprecated - needed for Minsky.1 capability
    typedef std::map<int, GodleyIcon> GodleyItems;
    GodleyItems godleyItems;

    // including a reference here ensures serialisation
    //  PortManager::Ports& ports;
    //  PortManager::Wires& wires;

    Operations operations;
    VariableManager variables;

    typedef map<int, GroupIcon> GroupIcons;
    GroupIcons groupItems;

    Plots plots;

    /// TCL accessors
    GetterSetter<int, Port> port;
    GetterSetter<int, Wire> wire;
    GetterSetter<int, Operation> op;
    GetterSetterPtr<int, VariablePtr> var;
    GetterSetter<string, VariableValue> value;
    GetterSetter<string, PlotWidget> plot;
    GetterSetter<int, GodleyIcon> godleyItem;
    GetterSetter<int, GroupIcon> groupItem;

    Minsky();
    ~Minsky() {clearAll();} //improve shutdown times

    void clearAll();

    using PortManager::closestPort;
    using PortManager::closestOutPort;
    using PortManager::closestInPort;

    array<float> portCoords(TCL_args);
    bool portInput(TCL_args);

    /// add a new wire connecting \a from port to \a to port with \a coordinates
    /// @return wireid, or -1 if wire is invalid
    int addWire(TCL_args args);
    void deleteWire(TCL_args args); 
    // get/set coordinates of a particular wire
    array<float> wireCoords(TCL_args args);


    /// list the possible string values of an enum (for TCL)
    template <class E> void enumVals()
    {
      tclreturn r;
      for (int i=0; i < sizeof(enum_keysData<E>::keysData) / sizeof(EnumKey); ++i)
        r << enum_keysData<E>::keysData[i].name;
    }


    /// list of available operations
    void availableOperations() {enumVals<OpAttributes::Type>();}

    /// return list of available asset classes
    void assetClasses() {enumVals<GodleyTable::AssetClass>();}

    /// add an operation
    int AddOperation(const char* op);
    int addOperation(TCL_args args) {return AddOperation(args);}
    /// create a new operation that is a copy of \a id
    int CopyOperation(int id);
    int copyOperation(TCL_args args) {return CopyOperation(args);}

    void DeleteOperation(int op);
    void deleteOperation(TCL_args args) {DeleteOperation(args);}

    /// useful for debugging wiring diagrams
    array<int> unwiredOperations();

    int newVariable(TCL_args args) {return variables.newVariable(args);}
    int CopyVariable(int id);
    int copyVariable(TCL_args args) {return CopyVariable(args);}

    int CopyGroup(int id);
    int copyGroup(TCL_args args) {return CopyGroup(args);}

    void deleteVariable(TCL_args args) {variables.erase(args);}

    void deletePlot(TCL_args args) {
      string image((char*)args);
      plots.plots[image].deletePorts();
      plots.plots.erase(image);
    }

    int addGodleyTable(TCL_args args) 
    {
      int id=godleyItems.empty()? 0: godleyItems.rbegin()->first+1;
      GodleyIcon& g = godleyItems[id];
      g.moveTo(args);
      g.table.doubleEntryCompliant=true;
      g.update();
      return id;
    }
    
    void deleteGodleyTable(TCL_args args)
    {
      GodleyItems::iterator g=godleyItems.find((int)args);
      if (g!=godleyItems.end())
        {
          for (GodleyIcon::Variables::iterator v=g->second.flowVars.begin();
               v!=g->second.flowVars.end(); ++v)
            variables.erase(*v);
          for (GodleyIcon::Variables::iterator v=g->second.stockVars.begin();
               v!=g->second.stockVars.end(); ++v)
            variables.erase(*v);
          godleyItems.erase((int)args);
        }
    }

    int group(TCL_args args);
    void ungroup(TCL_args args);
    

    /// add any newly created Godley variables. Variables removed from
    /// the Godley table will still exist with the system until deleted
    /// on the wiring canvas
    //  void updateGodleyVariables();

    /// evaluate the Godley table (update stock variables according to
    /// the current value of the internal variables
    void godleyEval(double sv[], const double fv[]);

    // runs over all ports and variables removing those not in use
    void garbageCollect();

    /// construct the equations based on input data
    /// @throws ecolab::error if the data is inconsistent
    void constructEquations();
    /// evaluate the equations (stockVars.size() of them)
    void evalEquations(double result[], const double vars[]);

    typedef MinskyMatrix Matrix; 
    void jacobian(Matrix& jac, const double vars[]);

    // Runge-Kutta parameters
    double stepMin; ///< minimum step size
    double stepMax; ///< maximum step size
    int nSteps;     ///< number of steps per GUI update
    double epsAbs;     ///< absolute error
    double epsRel;     ///< relative error

    double t; ///< time
    void reset(); ///<resets the variables back to their initial values
    void step();  ///< step the equations (by n steps, default 1)

    /// save to a file
    void Save(const char* filename);
    void save(TCL_args args) {Save(args);}
    /// load from a file
    void Load(const char* filename);
    void load(TCL_args args) {Load(args);}

    void latex(TCL_args args) {
      ofstream f(args);
      f<<"\\documentclass{article}\n\\begin{document}\n";
      MathDAG::SystemOfEquations(*this).latex(f);
      f<<"\\end{document}\n";
    }

    /// return the order in which operations are applied (for debugging purposes)
    array<int> opOrder(); 

    /// return the AEGIS assigned version number
    static const char* minskyVersion;
    string ecolabVersion() {return VERSION;}
  };
  
  extern Minsky minsky;

}

inline void pack(pack_t&, const string&,minsky::MinskyExclude&) {}
inline void unpack(pack_t&, const string&,minsky::MinskyExclude&) {}
inline void xml_pack(xml_pack_t&, const string&,minsky::MinskyExclude&) {}
inline void xml_unpack(xml_unpack_t&, const string&,minsky::MinskyExclude&) {}

#ifdef _CLASSDESC
#pragma omit pack minsky::MinskyExclude
#pragma omit unpack minsky::MinskyExclude
#pragma omit xml_pack minsky::MinskyExclude
#pragma omit xml_unpack minsky::MinskyExclude

  // we don't want to serialise this helper
#pragma omit pack minsky::GetterSetter
#pragma omit unpack minsky::GetterSetter
#pragma omit xml_pack minsky::GetterSetter
#pragma omit xml_unpack minsky::GetterSetter
#pragma omit pack minsky::GetterSetterPtr
#pragma omit unpack minsky::GetterSetterPtr
#pragma omit xml_pack minsky::GetterSetterPtr
#pragma omit xml_unpack minsky::GetterSetterPtr

#pragma omit pack minsky::MinskyMatrix
#pragma omit unpack minsky::MinskyMatrix
#pragma omit xml_pack minsky::MinskyMatrix
#pragma omit xml_unpack minsky::MinskyMatrix
#endif

template <class K, class T> 
void pack(classdesc::pack_t&,const string&,minsky::GetterSetter<K,T>&) {}
template <class K, class T> 
void unpack(classdesc::unpack_t&,const string&,minsky::GetterSetter<K,T>&) {}
template <class K, class T> 
void xml_pack(classdesc::xml_pack_t&,const string&,minsky::GetterSetter<K,T>&) {}
template <class K, class T> 
void xml_unpack(classdesc::xml_unpack_t&,const string&,minsky::GetterSetter<K,T>&) {}
template <class K, class T> 
void pack(classdesc::pack_t&,const string&,minsky::GetterSetterPtr<K,T>&) {}
template <class K, class T> 
void unpack(classdesc::unpack_t&,const string&,minsky::GetterSetterPtr<K,T>&) {}
template <class K, class T> 
void xml_pack(classdesc::xml_pack_t&,const string&,minsky::GetterSetterPtr<K,T>&) {}
template <class K, class T> 
void xml_unpack(classdesc::xml_unpack_t&,const string&,minsky::GetterSetterPtr<K,T>&) {}

#endif
