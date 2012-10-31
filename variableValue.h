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
#ifndef VARIABLE_VALUE
#define VARIABLE_VALUE
#include "variable.h"

namespace minsky
{
  class VariableValue
  {
    CLASSDESC_ACCESS(VariableValue);
  public:
    typedef VariableBase::Type Type;
  private:
    Type m_type;
    int m_idx; /// index into value vector
    double& valRef(); 

    friend class VariableManager;
    friend struct SchemaHelper;
  public:
    /// variable is on left hand side of flow calculation
    bool lhs() const {
      return m_type!=VariableBase::stock && m_type!=VariableBase::integral;}
    /// variable is a temporary
    bool temp() const {
      return type()==VariableType::tempFlow || type()==VariableType::undefined;}

    Type type() const {return m_type;}
    double init;
    bool godleyOverridden;

    double value() const {
      return const_cast<VariableValue*>(this)->valRef();
    }
    int idx() const {return m_idx;}

    VariableValue(Type type=VariableBase::undefined, double init=0): 
      m_type(type), m_idx(-1), init(init), godleyOverridden(0) {}

    const VariableValue& operator=(double x) {valRef()=x; return *this;}
    const VariableValue& operator+=(double x) {valRef()+=x; return *this;}
    const VariableValue& operator-=(double x) {valRef()-=x; return *this;}
    //  const VariableValue& operator+=(const VariableValue& x) {operator+=(x.value());}
    //  const VariableValue& operator-=(const VariableValue& x) {operator-=(x.value());}

    /// allocate space in the variable vector. @returns reference to this
    VariableValue& allocValue();
    void reset() {
      if (m_idx>=0) operator=(init);
    }
  };

  struct ValueVector
  {
    /// vector of variables that are integrated via Runge-Kutta. These
    /// variables label the columns of the Godley table
    static std::vector<double> stockVars;
    /// variables defined as a simple function of the stock variables,
    /// also known as lhs variables. These variables appear in the body
    /// of the Godley table
    static std::vector<double> flowVars;
  };
}
#include "variableValue.cd"
#endif
