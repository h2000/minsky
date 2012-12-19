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
#define OPNAMEDEF
#include "operation.h"
#include "variable.h"
#include "portManager.h"
#include "minsky.h"
#include "cairoItems.h"

#include <ecolab_epilogue.h>

#include <math.h>
using namespace minsky;

// necessary for Classdesc reflection!
const float IntOp::intVarOffset;
const float OpAttributes::l;
const float OpAttributes::h;
const float OpAttributes::r;


string OperationBase::name() const 
{return enumKey<Type>(type());}

string OperationBase::OpName(int op) 
{return enumKey<Type>(op);}

float OperationBase::x() const
{
  if (group>=0)
    return m_x + minsky::minsky.groupItems[group].x();
  else
    return m_x;
}

float OperationBase::y() const
{
  if (group>=0)
    return m_y + minsky::minsky.groupItems[group].y();
  else
    return m_y;
}

void OperationBase::MoveTo(float x1, float y1)
{
  float dx=x1-x(), dy=y1-y();
  move(dx,dy);
}

void OperationBase::move(float x1, float y1)
{
  m_x+=x1; m_y+=y1;
  for (size_t i=0; i<m_ports.size(); ++i)
    portManager().movePort(m_ports[i], x1, y1);
}

void OperationBase::addPorts()
{
  m_ports.clear();
  switch (type())
    {
      // zero input port case
    case constant: 
    case time: 
      m_ports.push_back(portManager().addPort(Port()));
      break;
      // single input port case
    case copy: case exp:
      m_ports.push_back(portManager().addPort(Port()));
      m_ports.push_back(portManager().addPort(Port(0,0,true)));
      break;
      // dual input port case
    case add: case subtract: 
    case multiply: case divide:
      m_ports.push_back(portManager().addPort(Port()));
      m_ports.push_back(portManager().addPort(Port(0,0,true)));
      assert(portManager().ports[m_ports.back()].input);
      m_ports.push_back(portManager().addPort(Port(0,0,true)));
      assert(portManager().ports[m_ports.back()].input);
      assert(portManager().ports.size()>2);
      break;
    }

}

IntOp::IntOp(const vector<int>& ports): Super(ports), intVar(-1) 
{
  if (ports.empty())
    {
      delPorts(); // to be sure, although there shouldn't be any ports
      addPorts();
    }
}

const IntOp& IntOp::operator=(const IntOp& x)
{
  Super::operator=(x); 
  intVar=-1;  // cause a new integral variable to be created
  m_description=x.m_description; 
  addPorts();
}

void IntOp::addPorts()
{
  m_ports.clear();
  setDescription();
  m_ports.push_back(portManager().addPort(Port(0,0,true)));
}


void OperationBase::delPorts()
{
  for (size_t i=0; i<m_ports.size(); ++i)
    portManager().delPort(m_ports[i]);
  m_ports.clear();
}

void IntOp::setDescription()
{
  if (type()!=integrate) return;
  vector<Wire> savedWires;
  if (numPorts()>0)
    {
      // save any attached wires for later use
      array<int> outWires=portManager().WiresAttachedToPort(m_ports[0]);
      for (array<int>::iterator i=outWires.begin(); i!=outWires.end(); ++i)
        savedWires.push_back(portManager().wires[*i]);
    }


  if (intVar > -1)
    {
      VariablePtr& v=variableManager()[intVar];
      if (!m_ports.empty() && m_ports[0]!=v->outPort())
        portManager().delPort(m_ports[0]);
      if (v->name!=m_description)
        variableManager().removeVariable(variableManager()[intVar]->name);
      else
        return; // nothing to be done
    }
  else if (!m_ports.empty())
    portManager().delPort(m_ports[0]);

  // set a default name if none given
  if (m_description.empty()) m_description="int";
  // generate a non-existing variable name, based on current value of name
  if (variableManager().values.count(m_description))
    {
      int i=1;
      ostringstream trialName;
      do
        trialName<<m_description<<i++;
      while (variableManager().values.count(trialName.str()));
      m_description=trialName.str();
    }

  VariablePtr iv(VariableType::integral, m_description);
  iv->visible=false; // we're managing our own display
  intVar=variableManager().addVariable(iv);

  // make the intVar outport the integral operator's outport
  if (m_ports.size()<1) m_ports.resize(1);

  m_ports[0]=iv->outPort();

  // restore any previously attached wire
  for (int i=0; i<savedWires.size(); ++i)
    {
      Wire& w=savedWires[i];
      w.from=m_ports[0];
      if (variableManager().addWire(w.to, w.from))
        portManager().addWire(w);
    }
}

bool OperationBase::selfWire(int from, int to) const
{
  bool r=false;
  if (numPorts()>1 && from==ports()[0])
    for (int i=1; !r && i<numPorts(); ++i) 
      r|=to==ports()[i];
  return r;
}

OperationBase* OperationBase::create(OperationType::Type type,
                                     const vector<int>& ports)
{
  switch (type)
  {
  case constant:
    return new Constant(ports);
  case time:
    return new Operation<time>(ports);
  case copy:
    return new Operation<copy>(ports);
  case integrate:
    return new IntOp(ports);
  case exp:
    return new Operation<exp>(ports);
  case add:
    return new Operation<add>(ports);
  case subtract:
    return new Operation<subtract>(ports);
  case multiply:
    return new Operation<multiply>(ports);
  case divide:
    return new Operation<divide>(ports);
  case numOps:  // default, do nothing op
    return new Operation<numOps>(ports);
  default:
    assert("Invalid operation type requested" && false);
    return NULL;
  }
}


void EvalOp::reset()
{
  if (Constant* c=dynamic_cast<Constant*>(state.get()))
    ValueVector::flowVars[out]=c->value;
}

int EvalOp::numArgs()
{
  switch (op)
    {
    case OperationType::constant:
    case OperationType::time:
      return 0;
    case OperationType::copy:
    case OperationType::integrate:
    case OperationType::exp:
      return 1;      
    case OperationType::add:
    case OperationType::subtract:
    case OperationType::multiply:
    case OperationType::divide:
      return 2;
    }
  assert(false); //shouldn't be here
  return 0;
}


void EvalOp::eval(double fv[], const double sv[])
{
  fv[out]=evaluate(flow1? fv[in1]: sv[in1], flow2? fv[in2]: sv[in2], fv);
};

void EvalOp::deriv(double df[], const double ds[], 
                   const double sv[], const double fv[])
{
  assert(out>=0 && out<ValueVector::flowVars.size());
  switch (numArgs()) 
    {
    case 0:
      df[out]=0;
      return;
    case 1:
      {
        assert(flow1 && in1<ValueVector::flowVars.size() || !flow1&&in1<ValueVector::stockVars.size());
        double x1=flow1? fv[in1]: sv[in1];
        double dx1=flow1? df[in1]: ds[in1];
        df[out] = dx1!=0? dx1 * d1(x1,0): 0;
        return;
      }
    case 2:
      {
        assert(flow1 && in1<ValueVector::flowVars.size() || !flow1&&in1<ValueVector::stockVars.size());
         assert(flow2 && in2<ValueVector::flowVars.size() || !flow2&&in2<ValueVector::stockVars.size());
       double x1=flow1? fv[in1]: sv[in1];
        double x2=flow2? fv[in2]: sv[in2];
        double dx1=flow1? df[in1]: ds[in1];
        double dx2=flow2? df[in2]: ds[in2];
        df[out] = (dx1!=0? dx1 * d1(x1,x2): 0) + 
          (dx2!=0? dx2 * d2(x1,x2): 0);
        return;
      }
    }
}

double EvalOp::evaluate(double in1, double in2, const double v[]) const
{
  assert(op != OperationType::integrate);
  switch (op)
    {
    case OperationType::constant:
      return dynamic_cast<Constant&>(*state).value;
    case OperationType::time:
      return minsky.t;
    case OperationType::copy:
      return in1;
    case OperationType::exp:
      return exp(in1);      
    case OperationType::add:
      return in1+in2;
    case OperationType::subtract:
      return in1-in2;
    case OperationType::multiply:
      return in1*in2;
    case OperationType::divide:
      return in1/in2;
    default:
      return 0;
    }
};

double EvalOp::d1(double x1, double x2)
{
  switch (op)
    {
    case OperationType::constant:
    case OperationType::time:
      return 0;
    case OperationType::copy:
      return 1;
    case OperationType::integrate:
      return x1;
    case OperationType::exp:
      return exp(in1);      
    case OperationType::add:
    case OperationType::subtract:
      return 1;
    case OperationType::multiply:
      return x2;
    case OperationType::divide:
      return 1/x2;
    }
  assert(false); // shouldn't be here
  return 0;
}

double EvalOp::d2(double x1, double x2)
{
  switch (op)
    {
    case OperationType::constant:
    case OperationType::time:
    case OperationType::copy:
    case OperationType::integrate:
    case OperationType::exp:
      return 0;
    case OperationType::add:
      return 1;
    case OperationType::subtract:
      return -1;
    case OperationType::multiply:
      return x1;
    case OperationType::divide:
      return -x1/(x2*x2);
    }
  assert(false); // shouldn't be here
  return 0;
}

array<int> Operations::visibleOperations() const
{
  array<int> ret;
  for (const_iterator i=begin(); i!=end(); ++i)
    if (i->second->visible)
      ret<<=i->first;
  return ret;
}


bool IntOp::toggleCoupled()
{
  if (type()!=integrate) return false;
  if (intVar==-1) setDescription();

  VariablePtr v=getIntVar();
  v->toggleInPort();

  assert(m_ports.size()==2);
  if (coupled()) 
    {
      // we are coupled, decouple variable
      assert(v->inPort()>=0);
      m_ports[0]=portManager().addPort(Port(x(),y(),false));
      portManager().addWire(Wire(m_ports[0],v->inPort()));
      v->visible=true;
      v->rotation=rotation;
      float angle=rotation*M_PI/180;
      float xoffs=r+intVarOffset+RenderVariable(v).width();
      v->MoveTo(x()+xoffs*cos(angle), y()+xoffs*sin(angle));
    }
  else
    {
      assert(v->inPort()==-1);
      portManager().delPort(m_ports[0]);
      m_ports[0]=v->outPort();
      v->visible=false;
    }
  return coupled();
}

void OperationBase::zoom(float xOrigin, float yOrigin,float factor)
{
  if (visible)
    {
      if (group==-1)
        {
          ::zoom(m_x,xOrigin,factor);
          ::zoom(m_y,yOrigin,factor);
        }
      else
        {
          m_x*=factor;
          m_y*=factor;
        }
      zoomFactor*=factor;
    }
}
