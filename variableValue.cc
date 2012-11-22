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
#include "variableValue.h"

namespace minsky
{
  std::vector<double> ValueVector::stockVars(1);
  std::vector<double> ValueVector::flowVars(1);

  VariableValue& VariableValue::allocValue()
  {
    switch (m_type)
      {
      case VariableBase::undefined:
        m_idx=-1;
        break;
      case VariableBase::flow:
      case VariableBase::tempFlow:
        m_idx=ValueVector::flowVars.size();
        ValueVector::flowVars.resize(ValueVector::flowVars.size()+1);
        *this=init;
        break;
      case VariableBase::stock:
      case VariableBase::integral:
        m_idx=ValueVector::stockVars.size();
        ValueVector::stockVars.resize(ValueVector::stockVars.size()+1);
        *this=init;
        break;
      }
    return *this;
  }

  double& VariableValue::valRef()
  {
    if (m_type==VariableBase::undefined || m_idx==-1)
      return init;
    switch (m_type)
      {
      case VariableBase::flow:
      case VariableBase::tempFlow:
        assert(m_idx<ValueVector::flowVars.size());
        return ValueVector::flowVars[m_idx];
      case VariableBase::stock:
      case VariableBase::integral:
        assert(m_idx<ValueVector::stockVars.size());
        return ValueVector::stockVars[m_idx];
      }
    return init;
  }


}
