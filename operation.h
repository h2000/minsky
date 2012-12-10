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

struct OperationType
{
  enum Type {constant, time, // zero input port ops
             copy, integrate, exp,     // single input port ops
             add, subtract, multiply, divide, // dual input port ops
             numOps // last operation, for iteration purposes
  };
};

namespace minsky
{
  using namespace ecolab;
  using namespace classdesc;
  using namespace std;
  //using ecolab::array_ns::array;

  class OpAttributes: public OperationType
  {
  public:

    // triangle parameters - l: xcoord of lhs, r; xcoord of apex, h: height of base
    static const float l=-8, h=12, r=12;

    float m_x, m_y, zoomFactor;
    // operator dependent data
    double rotation; /// rotation if icon, in degrees

    bool visible; ///< whether operation is visible on Canvas 
    /// contains group ID if this is a group item, -1
    /// otherwise. Coordinates are taken relative to group centre, if
    /// this is a group item
    int group;
    OpAttributes(): m_x(10), m_y(10), zoomFactor(1), rotation(0), visible(true), group(-1) {}
  };

  class OperationBase: public classdesc::PolyBase<OperationType::Type>,
                       public OpAttributes
  {
    CLASSDESC_ACCESS(OperationBase);
  public:
    typedef OpAttributes::Type Type;
    //    virtual Type type() const=0;
    const vector<int>& ports() const {return m_ports;}
    int numPorts() const  {return m_ports.size();}
    ///factory method. \a ports is used for recreating an object read
    ///from a schema
    static OperationBase* create(Type type, 
                                 const vector<int>& ports = vector<int>()); 
    virtual OperationBase* clone() const=0;

    OperationBase() {}
    OperationBase(const OpAttributes& attr): OpAttributes(attr) {}
    OperationBase(const vector<int>& ports): m_ports(ports) {}
    virtual ~OperationBase() {}

    /// return the symbolic name of this operation's type
    string name() const;
    /// return the symbolic name of \a type
    static string OpName(int type);
    /// return the symbolic name of \a type
    static string opName(TCL_args args) {return OpName(args);}

    /// override of coordinate attributes, allowing group offsets to
    /// be converted to canvas coordinates
    float x() const;
    float y() const;

    // relative move
    void move(float dx, float dy); 
    /// move whole operation object to canvas location x,y
    void MoveTo(float x, float y); 
    void moveTo(TCL_args args) {MoveTo(args[0], args[1]);}

    /// zoom by \a factor, scaling all widget's coordinates, using (\a
    /// xOrigin, \a yOrigin) as the origin of the zoom transformation
    void zoom(float xOrigin, float yOrigin,float factor);
    void setZoom(float factor) {zoomFactor=factor;}

    /// returns true if from matches the out port, and to matches one of
    /// the in ports
    bool selfWire(int from, int to) const;

  protected:
    // manage the port structures associated with this operation
    virtual void addPorts();
    void addPorts(const vector<int>& ports) {
      if (!ports.empty())
        // TODO - possible consistency check possible here
        m_ports=ports; 
      else
        addPorts(); 
    }
    void delPorts();

    vector<int> m_ports;
    friend struct EvalOp;
  };

  template <OperationType::Type T>
  class Operation: public classdesc::PolyBaseT<Operation<T>, OperationBase>
  {
    typedef classdesc::PolyBaseT<Operation<T>, OperationBase> Super;
  public:
    typedef OperationType::Type Type;
    Type type() const {return T;}
//    Operation* clone() const {
//      return new Operation(*this);
//    }

    // ensure copies create new ports
    Operation(const Operation& x): Super(x) {this->addPorts();}
    const Operation& operator=(const Operation& x)
    {Super::operator=(x); this->addPorts();}

    Operation() {this->addPorts();}
    Operation(const vector<int>& ports) {this->addPorts(ports);}
    ~Operation() {this->delPorts();}
  };

  class Constant: public Operation<OperationType::constant>
  {
    typedef Operation<OperationType::constant> Super;
  public:
    // constants have sliders
    bool sliderVisible, ///< slider is visible on canvas
      sliderBoundsSet, ///< slider bounds have been initialised at some point
      sliderStepRel;   /**< sliderStep is relative to the range
                          [sliderMin,sliderMax] */

    double sliderMin, sliderMax, sliderStep;
    string description; ///< constant name
    double value; ///< constant value
    Constant(const vector<int>& ports=vector<int>()):  
      Super(ports), value(0), sliderVisible(false), sliderBoundsSet(false), 
      sliderStepRel(false) {}

    // clone has to be overridden, as default impl return object of
    // type Operation<T>
    Constant* clone() const {return new Constant(*this);}
  };

  class IntOp: public Operation<OperationType::integrate>
  {
    typedef Operation<OperationType::integrate> Super;
    // integrals have named integration variables
    ///integration variable associated with this op. -1 if not used
    int intVar; 
    /// name of integration variable
    string m_description; 
    CLASSDESC_ACCESS(IntOp);
    void addPorts(); // override. Also allocates new integral var if intVar==-1
    friend struct SchemaHelper;
  public:
    // offset for coupled integration variable, tr
    static const float intVarOffset=10;

    IntOp(): intVar(-1) {}
    IntOp(const vector<int>& ports);

    // ensure that copies create a new integral variable
    IntOp(const IntOp& x): 
      Super(x), intVar(-1), m_description(x.m_description)  {addPorts();}
    const IntOp& operator=(const IntOp& x); 

    ~IntOp() {variableManager().erase(intVarID());}

    // clone has to be overridden, as default impl return object of
    // type Operation<T>
    IntOp* clone() const {return new IntOp(*this);}

    /// set integration variable name
    void setDescription();
    /// description pretends to be an attribute for TCL purposes
    void setDescription(const string& desc) {
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

    /// toggles coupled state of integration variable. Only valid for integrate
    /// @return coupled state
    bool toggleCoupled();
    bool coupled() const {
      return intVar>-1 && ports().size()>1 && ports()[0]==getIntVar()->outPort();
    }

  };

  /// shared_ptr class for polymorphic operation objects. Note, you
  /// may assume that this pointer is always valid, although currently
  /// the implementation doesn't guarantee it (eg reset() is exposed).
  class OperationPtr: public shared_ptr<OperationBase>
  {
  public:
    OperationPtr(OperationType::Type type=OperationType::numOps,
                 const vector<int>& ports=vector<int>()): 
      shared_ptr<OperationBase>(OperationBase::create(type, ports)) {}
    // reset pointer to a newly created operation
    OperationPtr(OperationBase* op): shared_ptr<OperationBase>(op) 
    {assert(op);}
    OperationPtr clone() const {return OperationPtr(get()->clone());}
  };


  /// represents the operation when evaluating the equations
  struct EvalOp
  {
    OperationType::Type op;
    /// indexes into the Godley variables vector
    int out, in1, in2;
    ///indicate whether in1/in2 are flow variables (out is always a flow variable)
    bool flow1, flow2; 

    /// state data (for those ops that need it)
    OperationPtr state;

    EvalOp() {}
    EvalOp(OperationType::Type op, int out, int in1=0, int in2=0, 
           bool flow1=true, bool flow2=true): 
      op(op), out(out), in1(in1), in2(in2), flow1(flow1), flow2(flow2) 
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

  struct Operations: public map<int, OperationPtr>
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
