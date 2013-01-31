/*
  @copyright Steve Keen 2013
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
#ifndef EVALOP_H
#define EVALOP_H

#include <ecolab.h>
#include <xml_pack_base.h>
#include <xml_unpack_base.h>

#include "variableValue.h"
#include "variableManager.h"
#include "operation.h"

#include <vector>
#include <cairo/cairo.h>

#include <arrays.h>

// override EcoLab's default CLASSDESC_ACCESS macro
#include "classdesc_access.h"


namespace minsky
{
  using namespace ecolab;
  using namespace classdesc;
  using namespace std;
  //using ecolab::array_ns::array;


  struct EvalOpBase: public classdesc::PolyBase<OperationType::Type>,
                     public OperationType
  {
    /// indexes into the Godley variables vector
    int out, in1, in2;
    ///indicate whether in1/in2 are flow variables (out is always a flow variable)
    bool flow1, flow2; 

    /// state data (for those ops that need it)
    OperationPtr state;
    EvalOpBase(int out=0, int in1=0, int in2=0, 
               bool flow1=true, bool flow2=true): 
      out(out), in1(in1), in2(in2), flow1(flow1), flow2(flow2) 
    {}
    virtual ~EvalOpBase() {}

    /// factory method
    static EvalOpBase* create
    (Type op=numOps, int out=0, int in1=0, int in2=0, 
     bool flow1=true, bool flow2=true);

    /// reset state to initial values
    void reset();
    /// number of arguments to this operation
    virtual int numArgs() const =0;
    /// evaluate expression on sv and current value of fv, storing result
    /// in output variable (of \a fv)
    void eval(double fv[]=&ValueVector::flowVars[0], 
              const double sv[]=&ValueVector::stockVars[0]);
 
    /// evaluate expression on given arguments, returning result
    virtual double evaluate(double in1=0, double in2=0) const=0;
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
    virtual double d1(double x1=0, double x2=0) const=0;
    virtual double d2(double x1=0, double x2=0) const=0;
    /// @}
  };

  /// represents the operation when evaluating the equations
  template <OperationType::Type T>
  struct EvalOp: public classdesc::PolyBaseT<EvalOp<T>, EvalOpBase>
  {
    EvalOp(int out=0, int in1=0, int in2=0, 
           bool flow1=true, bool flow2=true) 
    {
      // Cannot directly initialise base class, as PolyBaseT intervenes
      this->out=out;
      this->in1=in1; this->in2=in2;
      this->flow1=flow1; this->flow2=flow2;
    }

    OperationType::Type type() const {return T;}
    int numArgs() const;
    double evaluate(double in1=0, double in2=0) const;
    double d1(double x1=0, double x2=0) const;
    double d2(double x1=0, double x2=0) const;
   
  };

  struct EvalOpPtr: public classdesc::shared_ptr<EvalOpBase>, 
                    public OperationType
  {
    /// factory method
    EvalOpPtr(OperationType::Type op=OperationType::numOps, 
              int out=0, int in1=0, int in2=0, 
              bool flow1=true, bool flow2=true);
  };
}

#include "evalOp.cd"
#endif
