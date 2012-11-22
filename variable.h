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
#ifndef VARIABLE_H
#define VARIABLE_H

#include <ecolab.h>
#include <arrays.h>
using namespace ecolab;
using ecolab::array_ns::array;

#include <vector>
#include <map>
// override EcoLab's default CLASSDESC_ACCESS macro
#include "classdesc_access.h"

#include "polyBase.h"

namespace minsky 
{
  class VariablePtr;
}

struct VariableType
{
  enum Type {undefined, flow, stock, tempFlow, integral};
};

struct VariableBaseAttributes: public VariableType
{
  VariableBaseAttributes(): x(0), y(0), rotation(0), visible(true) {}

  // tempFlow variables are temporary flow variable not visible on
  // the canvas.
  // integral variables are temporary stock variables used
  // to implement integration

  float x, y; ///< position in canvas
  string name; ///< variable name
  double rotation; /// rotation if icon, in degrees
  
  /**
     whether variable is visible on Canvas (note godley variables are
     never visible, as they appear as part of the godley icon 
  */
  bool visible;
};

namespace minsky {struct SchemaHelper;}

class VariableBase: public classdesc::PolyBase<VariableType::Type>,
                      public VariableBaseAttributes
{
public:
protected:
  
  void delPorts();
  void addPorts();
  friend class minsky::VariablePtr;
  friend struct minsky::SchemaHelper;
private:
  int m_outPort, m_inPort; /// where wires connect to
  CLASSDESC_ACCESS(VariableBase);
 
public:
  /// variable is in a Godley table
    bool m_godley;
  
  int outPort() const {return m_outPort;}
  int inPort() const {return m_inPort;}
  virtual int numPorts() const=0;
  array<int> ports() const; 
  static VariableBase* create(Type type); ///factory method
  
  double Init() const; /// < return initial value for this variable
  double Init(double); /// < set the initial value for this variable
  double init(TCL_args args) {
    if (args.count) return Init(args);
    else return Init();
  }

  /// variable is on left hand side of flow calculation
  bool lhs() const {return type()!=stock && type()!=integral;} 
  /// variable is temporary
  bool temp() const {return type()==tempFlow || type()==undefined;}
  //TODO: remove leading m_
  virtual Type type() const=0;
  virtual VariableBase* clone() const=0;
  
  VariableBase(const string& name=""): 
    m_outPort(-1), m_inPort(-1), m_godley(false) {
    this->name=name;
  }
  VariableBase(const VariableBase& x): 
    classdesc::PolyBase<VariableType::Type>(x),
    VariableBaseAttributes(x), m_outPort(-1), m_inPort(-1), m_godley(false) {}
  virtual ~VariableBase() {}
  
  void move(float dx, float dy); ///< relative move
  void MoveTo(float x1, float y1); ///< absolute move
  void moveTo(TCL_args args) {MoveTo(args[0], args[1]);}
  
  /// adds inPort for integral case (not relevant elsewhere) if one
  /// not allocated, removes it if one allocated
  void toggleInPort();
  
};

namespace minsky
{
  template <VariableBase::Type T>
  class Variable: public classdesc::PolyBaseT<Variable<T>, VariableBase>
  {
  public:
    typedef VariableBase::Type Type;
    Type type() const {return T;}
    int numPorts() const;
    Variable(const string& name="") 
    {this->name=name;}
    ~Variable() {this->delPorts();}
    // clones the current object, allocating new ports
    Variable* clone() const {
      Variable* v=new Variable(*this);
      v->addPorts();
      return v;
    }
  };

  class VariablePtr: 
    public classdesc::shared_ptr<VariableBase>
  {
    typedef classdesc::shared_ptr<VariableBase> PtrBase;
  public:
    VariablePtr(VariableBase::Type type=VariableBase::undefined, 
                const std::string& name=""): 
      PtrBase(VariableBase::create(type)) {get()->name=name; get()->addPorts();}
    template <class P>
    VariablePtr(P* var): PtrBase(dynamic_cast<VariableBase*>(var)) 
    {
      // check for incorrect type assignment
      assert(!var || *this);
    }
    VariablePtr(const VariableBase& x): PtrBase(x.clone()) {}
  };

}
#include "variable.cd"
#endif
