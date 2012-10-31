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
#ifndef OPERATION_H
#define OPERATION_H

#include <ecolab.h>
#include <xml_pack_base.h>
#include <xml_unpack_base.h>

#include "variableValue.h"
#include "variableManager.h"

#include <vector>

#include <arrays.h>

// override EcoLab's default CLASSDESC_ACCESS macro
#include "classdesc_access.h"

namespace minsky
{
  using namespace ecolab;
  using namespace classdesc;
  using namespace std;
  //using ecolab::array_ns::array;

  class OpAttributes
  {
    CLASSDESC_ACCESS(OpAttributes);

  public:
    enum Type {constant, time, // zero input port ops
               copy, integrate, exp,     // single input port ops
               add, subtract, multiply, divide, // dual input port ops
               numOps // last operation, for iteration purposes
    };


    // offset for coupled integration variable, tr
    static const float intVarOffset=10;
    // triangle parameters - l: xcoord of lhs, r; xcoord of apex, h: height of base
    static const float l=-8, h=12, r=12;

    float x, y;
    // operator dependent data
    double value; /// for constants
    double rotation; /// rotation if icon, in degrees

    bool visible; ///< whether operation is visible on Canvas 
    Type m_type;

    bool sliderVisible, sliderBoundsSet, sliderStepRel;
    double sliderMin, sliderMax, sliderStep;
    OpAttributes(): x(10), y(10), m_type(numOps), value(0), rotation(0),
                    visible(true), sliderVisible(false), 
                    sliderBoundsSet(false), sliderStepRel(false) {}
  };

  class Operation: public OpAttributes
  {
    CLASSDESC_ACCESS(Operation);
    friend struct SchemaHelper;
  public:
    typedef OpAttributes::Type Type;
    Type type() const {return m_type;}
    const vector<int>& ports() const {return m_ports;}
    int numPorts() const {return m_ports.size();}
    // manage the port structures associated with this operation
    void addPorts();
    void delPorts();

    Operation(Type op=numOps): intVar(-1) {m_type=op;}
    Operation(const OpAttributes& attr): OpAttributes(attr), intVar(-1) {}

    /// return the symbolic name of this operation's type
    string name() const;
    /// return the symbolic name of \a type
    static string OpName(int type);
    /// return the symbolic name of \a type
    static string opName(TCL_args args) {return OpName(args);}

    /// move whole operation object to canvas location x,y
    void MoveTo(float x, float y); 
    void moveTo(TCL_args args) {MoveTo(args[0], args[1]);}

    /// set integration variable name
    void setDescription();
    /// description pretends to be an attribute for TCL purposes
    void setDescription(string desc) {
      m_description=desc;
      setDescription();
    }

    string description(TCL_args args=TCL_args()) {
      if (args.count) {setDescription(args);}
      return m_description;
    }

    /// we cannot overload description with aconst version, as this
    /// would destroy it ability to be called from TCL
    const string& getDescription() const {return m_description;}

    /// return ID of integration variable
    int intVarID() const {return intVar;}

    /// return reference to integration variable
    VariablePtr getIntVar() const {
      if (intVar>-1)
        return variableManager()[intVar];
      else
        return VariablePtr();
    }

    /// returns true if from matches the out port, and to matches one of
    /// the in ports
    bool selfWire(int from, int to) const;

    /// toggles coupled state of integration variable. Only valid for integrate
    /// @return coupled stated
    bool toggleCoupled();
    bool coupled() const {
      return intVar>-1 && m_ports.size()>1 && m_ports[0]==getIntVar()->outPort();
    }


  private:
    string m_description; ///name of constant, variable, or UNURAN string for RNG,
    vector<int> m_ports;
    friend struct EvalOp;
    ///integration variable associated with this op. -1 if not used
    int intVar; 
  };

  /// represents the operation when evaluating the equations
  struct EvalOp
  {
    Operation::Type op;
    /// indexes into the Godley variables vector
    int out, in1, in2;
    ///indicate whether in1/in2 are flow variables (out is always a flow variable)
    bool flow1, flow2; 

    /// state data (for those ops that need it)
    Operation* state;

    EvalOp() {}
    EvalOp(Operation::Type op, int out, int in1=0, int in2=0, 
           bool flow1=true, bool flow2=true): 
      op(op), out(out), in1(in1), in2(in2), state(0), flow1(flow1), flow2(flow2) 
    {}

    /// reset state to initial values
    void reset();
    /// number of arguments to this operation
    int numArgs();
    /// evaluate expression on sv and current value of fv, storing result
    /// in output variable (of \a fv)
    void eval(double fv[]=&ValueVector::flowVars[0], 
              const double sv[]=&ValueVector::stockVars[0]); 
    /// evaluate expression on given arguments, returning result
    double evaluate(double in1=0, double in2=0, 
                    const double v[]=&ValueVector::flowVars[0]) const;
    /**
       total derivate with respect to a variable, which is a function of the stock variables.
       @param sv - stock variables
       @param fv - flow variables (function of stock variables, computed by eval)
       @param ds - derivative of stock variables
       @param df - derivative of flow variables (updated by this function)

       To compute the partial derivatives with respect to stock variable
       i, seed ds with 1 in the ith position, 0 every else, and
       initialise df to zero.
    */
    void deriv(double df[], const double ds[], 
               const double sv[], const double fv[]);
    /**
       @{
       derivatives with respect to 1st and second argument
    */
    double d1(double x1=0, double x2=0);
    double d2(double x1=0, double x2=0);
    /// @}
  };

  struct Operations: public map<int, Operation>
  {
    array<int> visibleOperations() const;
  };

}

  // for TCL interfacing
inline std::ostream& operator<<(std::ostream& x, const std::vector<int>& y)
{
  for (size_t i=0; i<y.size(); ++i)
    x<<(i==0?"":" ")<<y[i];
  return x;
}


#ifdef _CLASSDESC
#pragma omit pack minsky::EvalOp
#pragma omit unpack minsky::EvalOp 
#pragma omit xml_pack minsky::EvalOp
#pragma omit xml_unpack minsky::EvalOp
#endif

inline void pack(classdesc::pack_t&,const classdesc::string&,classdesc::ref<urand>&) {}
inline void unpack(classdesc::pack_t&,const classdesc::string&,classdesc::ref<urand>&) {}
inline void xml_pack(classdesc::xml_pack_t&,const classdesc::string&,classdesc::ref<urand>&) {}
inline void xml_unpack(classdesc::xml_unpack_t&,const classdesc::string&,classdesc::ref<urand>&) {}

#include "operation.cd"
#endif
