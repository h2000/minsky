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
#include "../minsky.h"
#include <UnitTest++/UnitTest++.h>
using namespace minsky;

namespace
{
  struct TestFixture: public Minsky
  {
    TestFixture()
    {
      setPortManager(*this);
      setVariableManager(variables);
      GodleyIcon::setMinsky(*this);
    }
    void addWire(const Wire& w)
    {
      CHECK(ports.count(w.from));
      CHECK(ports.count(w.to));
      CHECK(!ports[w.from].input && ports[w.to].input);
      CHECK(variables.addWire(w.from, w.to));
      PortManager::addWire(w);
    }
  };
}

/*
  ASCII Art diagram for the below test:


     c           -- a
       \       /
        +--int 
       /       \
     d          * - b
               /
     e ------------ f
 */

TEST_FIXTURE(TestFixture,constructEquationsEx1)
{
  GodleyTable& godley=godleyItems[0].table;
  godley.Resize(3,4);
  godley.cell(0,1)="c";
  godley.cell(0,2)="d";
  godley.cell(0,3)="e";
  godley.cell(2,1)="a";
  godley.cell(2,2)="b";
  godley.cell(2,3)="f";
  godleyItems[0].update();

  // build a table of variables - names will be unique at this stage
  map<string, VariablePtr> var;
  for (VariableManager::iterator v=variables.begin(); v!=variables.end(); ++v)
    {
      var[v->second->name]=v->second;
      CHECK(v->second->m_godley);
    }

  CHECK(var["a"]->lhs());
  CHECK(var["b"]->lhs());
  CHECK(!var["c"]->lhs());
  CHECK(!var["d"]->lhs());
  CHECK(!var["e"]->lhs());
  CHECK(var["f"]->lhs());

  operations[1]=OperationPtr(OperationType::add);
  CHECK_EQUAL(3, operations[1]->numPorts());
  operations[2]=OperationPtr(OperationType::integrate);
  CHECK_EQUAL(2, operations[2]->numPorts());
  operations[3]=OperationPtr(OperationType::multiply);
  CHECK_EQUAL(3, operations[3]->numPorts());
 
  addWire(Wire(var["e"]->outPort(), var["f"]->inPort()));
  addWire(Wire(var["c"]->outPort(), operations[1]->ports()[1]));
  addWire(Wire(var["d"]->outPort(), operations[1]->ports()[2]));;
  addWire(Wire(operations[1]->ports()[0], operations[2]->ports()[1]));
  addWire(Wire(operations[2]->ports()[0], var["a"]->inPort()));
  addWire(Wire(operations[2]->ports()[0], operations[3]->ports()[1]));
  addWire(Wire(var["e"]->outPort(), operations[3]->ports()[2]));
  addWire(Wire(operations[3]->ports()[0], var["b"]->inPort()));

  for (PortManager::Wires::const_iterator w=wires.begin(); w!=wires.end(); ++w)
    {
      CHECK(!ports[w->second.from].input);
      CHECK(ports[w->second.to].input);
    }

  Save("constructEquationsEx1.xml");

  constructEquations();

  CHECK_EQUAL(operations.size()+1, equations.size());

  // first is the variable to variable copy
  CHECK_EQUAL("copy", OperationBase::OpName(equations[0].op));
  CHECK_EQUAL(variables.values["e"].idx(), equations[0].in1);
  CHECK_EQUAL(variables.values["f"].idx(), equations[0].out);

  // check that the integral has been changed to a copy
  CHECK_EQUAL(1, integrals.size());
  CHECK_EQUAL("copy", OperationBase::OpName(equations[1].op));
  CHECK_EQUAL(integrals[0].stock.idx(), equations[1].in1);
  CHECK_EQUAL(variables.values["a"].idx(), equations[1].out);

  CHECK_EQUAL("add", OperationBase::OpName(equations[2].op));
  CHECK_EQUAL(variables.values["c"].idx(), equations[2].in1);
  CHECK_EQUAL(variables.values["d"].idx(), equations[2].in2);

  CHECK_EQUAL("multiply", OperationBase::OpName(equations[3].op));
  CHECK_EQUAL(integrals[0].stock.idx(), equations[3].in1);
  CHECK_EQUAL(variables.values["e"].idx(), equations[3].in2);
  CHECK_EQUAL(variables.values["b"].idx(), equations[3].out);

  

}

/*
  ASCII Art diagram for the below test:

        K----------g
          \          
           +--------h
          /
         K
*/

TEST_FIXTURE(TestFixture,constructEquationsEx2)
{
  GodleyTable& godley=godleyItems[0].table;
  godley.Resize(4,2);
  godley.cell(2,1)="g";
  godley.cell(3,1)="h";
  godleyItems[0].update();

  // build a table of variables - names will be unique at this stage
  map<string, VariablePtr> var;
  for (VariableManager::iterator v=variables.begin(); v!=variables.end(); ++v)
    {
      var[v->second->name]=v->second;
      CHECK(v->second->m_godley);
    }

  operations[4]=OperationPtr(OperationType::constant);
  CHECK_EQUAL(1, operations[4]->numPorts());
  operations[5]=OperationPtr(OperationType::constant);
  CHECK_EQUAL(1, operations[5]->numPorts());
  operations[6]=OperationPtr(OperationType::add);
  CHECK_EQUAL(3, operations[6]->numPorts());

  wires[8]=Wire(operations[4]->ports()[0], var["g"]->inPort());
  wires[9]=Wire(operations[4]->ports()[0], operations[6]->ports()[1]);
  wires[10]=Wire(operations[5]->ports()[0], operations[6]->ports()[2]);
  wires[11]=Wire(operations[6]->ports()[0], var["h"]->inPort());

  for (PortManager::Wires::const_iterator w=wires.begin(); w!=wires.end(); ++w)
    {
      CHECK(!ports[w->second.from].input);
      CHECK(ports[w->second.to].input);
    }

  constructEquations();

  CHECK_EQUAL(3, equations.size());

  CHECK_EQUAL("constant", OperationBase::OpName(equations[0].op));
  CHECK_EQUAL(variables.values["g"].idx(), equations[0].out);

  CHECK_EQUAL("constant", OperationBase::OpName(equations[1].op));

  CHECK_EQUAL("add", OperationBase::OpName(equations[2].op));
  CHECK_EQUAL(variables.values["g"].idx(), equations[2].in1);
  CHECK_EQUAL(variables.values["h"].idx(), equations[2].out);


}

TEST_FIXTURE(TestFixture,godleyEval)
{
  GodleyTable& godley=godleyItems[0].table;
    godley.Resize(3,4);
    godley.cell(0,1)="c";
    godley.cell(0,2)="d";
    godley.cell(0,3)="e";
    godley.cell(2,1)="a";
    godley.cell(2,2)="-a";
    godleyItems[0].update();

    variables.values["c"].init=10;
    variables.values["d"].init=20;
    variables.values["e"].init=30;
    variables.values["a"].init=5;

    garbageCollect();
    reset();
    CHECK_EQUAL(10,variables.values["c"].value());
    CHECK_EQUAL(20,variables.values["d"].value());
    CHECK_EQUAL(30,variables.values["e"].value());
    CHECK_EQUAL(5,variables.values["a"].value());
    for (size_t i=0; i<stockVars.size(); ++i)
      stockVars[i]=0;
    godleyEval(&stockVars[0], &flowVars[0]);
    CHECK_EQUAL(5,variables.values["c"].value());
    CHECK_EQUAL(-5,variables.values["d"].value());
    CHECK_EQUAL(0,variables.values["e"].value());
    CHECK_EQUAL(5,variables.values["a"].value());
   
}

/*
  ASCII Art diagram for the below test:


     c           -- a
       \       /
        +--int 
       /       \
     d          * - b
               /
     e ------------ f
 */
TEST_FIXTURE(TestFixture,derivative)
{
  GodleyTable& godley=godleyItems[0].table;
  garbageCollect();

  godley.Resize(3,4);
  godley.cell(0,1)="c";
  godley.cell(0,2)="d";
  godley.cell(0,3)="e";
  godley.cell(2,1)="a";
  godley.cell(2,2)="b";
  godley.cell(2,3)="f";
  godleyItems[0].update();

  // build a table of variables - names will be unique at this stage
  map<string, VariablePtr> var;
  for (VariableManager::iterator v=variables.begin(); v!=variables.end(); ++v)
    {
      var[v->second->name]=v->second;
      CHECK(v->second->m_godley);
    }

  operations[1]=OperationPtr(OperationType::add);
  CHECK_EQUAL(3, operations[1]->numPorts());
  operations[2]=OperationPtr(OperationType::integrate);
  CHECK_EQUAL(2, operations[2]->numPorts());
  operations[3]=OperationPtr(OperationType::multiply);
  CHECK_EQUAL(3, operations[3]->numPorts());
 
  wires[7]=Wire(var["e"]->outPort(), var["f"]->inPort());
  wires[6]=Wire(var["c"]->outPort(), operations[1]->ports()[1]);
  wires[5]=Wire(var["d"]->outPort(), operations[1]->ports()[2]);
  wires[4]=Wire(operations[1]->ports()[0], operations[2]->ports()[1]);
  wires[3]=Wire(operations[2]->ports()[0], var["a"]->inPort());
  wires[2]=Wire(operations[2]->ports()[0], operations[3]->ports()[1]);
  wires[1]=Wire(var["e"]->outPort(), operations[3]->ports()[2]);
  wires[0]=Wire(operations[3]->ports()[0], var["b"]->inPort());

  constructEquations();
  vector<double> j(stockVars.size()*stockVars.size());
  Matrix jac(stockVars.size(),&j[0]);

  VariableValue& c=variables.values["c"];   c=100;
  VariableValue& d=variables.values["d"];   d=200;
  VariableValue& e=variables.values["e"];   e=300;
  double& x=stockVars.back();   x=0; // temporary variable storing \int c+d

  CHECK_EQUAL(4, stockVars.size());

  jacobian(jac,&stockVars[0]);
  
  CHECK_EQUAL(0, jac(0,0));
  CHECK_EQUAL(0, jac(0,1));  
  CHECK_EQUAL(0, jac(0,2));
  CHECK_EQUAL(1, jac(0,3));
  CHECK_EQUAL(0, jac(1,0));
  CHECK_EQUAL(0, jac(1,1));
  CHECK_EQUAL(x, jac(1,2));
  CHECK_EQUAL(e.value(), jac(1,3));
  CHECK_EQUAL(0,jac(2,0));
  CHECK_EQUAL(0,jac(2,1));
  CHECK_EQUAL(1,jac(2,2));
  CHECK_EQUAL(0,jac(2,3));
  CHECK_EQUAL(1,jac(3,0));
  CHECK_EQUAL(1,jac(3,1));
  CHECK_EQUAL(0,jac(3,2));
  CHECK_EQUAL(0,jac(3,3));
}

TEST_FIXTURE(TestFixture,integrals)
{
  // First, integrate a constant
  operations[1]=OperationPtr(OperationType::constant);
  operations[2]=OperationPtr(OperationType::integrate);
  int var=variables.addVariable(VariablePtr(VariableType::flow,"output"));
  wires[0]=Wire(operations[1]->ports()[0], operations[2]->ports()[1]);
  wires[1]=Wire(operations[2]->ports()[0], variables[var]->inPort());

  constructEquations();
  double& value = dynamic_cast<Constant*>(operations[1].get())->value;
  value=10;
  nSteps=1;
  step();
  CHECK_EQUAL(value*t, integrals[0].stock.value());
  double prevInt=integrals[0].stock.value();
  step();
  CHECK_EQUAL(prevInt, variables.values["output"].value());

  // now integrate the linear function
  operations[3]=OperationPtr(OperationType::integrate);
  wires[2]=Wire(operations[2]->ports()[0], operations[3]->ports()[1]);
  reset();
  step();
  CHECK_EQUAL(0.5*value*t*t, integrals[1].stock.value());
}

/*
  check that cyclic networks throw an exception

  a
    \ 
     + - a
    /
  b
*/

TEST_FIXTURE(TestFixture,cyclicThrows)
{
  // First, integrate a constant
  operations[1]=OperationPtr(OperationType::add);
  int w=variables.addVariable(VariablePtr(VariableType::flow,"w"));
  int a=variables.addVariable(VariablePtr(VariableType::flow,"a"));
  CHECK(variables.addWire(operations[1]->ports()[0], variables[w]->inPort()));
  CHECK(variables.addWire(variables[w]->outPort(), operations[1]->ports()[1]));
  PortManager::addWire(Wire(operations[1]->ports()[0], variables[w]->inPort()));
  PortManager::addWire(Wire(variables[w]->outPort(), operations[1]->ports()[1]));
  PortManager::addWire(Wire(variables[a]->outPort(), operations[1]->ports()[2]));

  CHECK_THROW(constructEquations(), ecolab::error);
}

/*
  but integration is allowed to cycle

   +--------+ 
    \        \
     *- int---+
    /
  b
*/

TEST_FIXTURE(TestFixture,cyclicIntegrateDoesntThrow)
{
  // First, integrate a constant
  operations[1]=OperationPtr(OperationType::integrate);
  operations[2]=OperationPtr(OperationType::multiply);
  int a=variables.addVariable(VariablePtr(VariableType::flow,"a"));
  CHECK(variables.addWire(operations[1]->ports()[0], operations[2]->ports()[1]));
  CHECK(variables.addWire(operations[2]->ports()[0], operations[1]->ports()[1]));
  CHECK(variables.addWire(variables[a]->outPort(), operations[2]->ports()[2]));

  PortManager::addWire(Wire(operations[1]->ports()[0], operations[2]->ports()[1]));
  PortManager::addWire(Wire(operations[2]->ports()[0], operations[1]->ports()[1]));
  PortManager::addWire(Wire(variables[a]->outPort(), operations[2]->ports()[2]));

  constructEquations();
}

TEST_FIXTURE(TestFixture,godleyIconVariableOrder)
{
  GodleyIcon& g=godleyItems[0];
  g.table.Dimension(3,4);
  g.table.cell(0,1)="a1";
  g.table.cell(0,2)="z2";
  g.table.cell(0,3)="d1";
  g.table.cell(2,1)="b3";
  g.table.cell(2,2)="x1";
  g.table.cell(2,3)="h2";
  g.update();
  
  assert(g.stockVars.size()==g.table.cols()-1 && g.flowVars.size()==g.table.cols()-1);
  for (int i=1; i<g.table.cols(); ++i)
    {
      CHECK_EQUAL(g.table.cell(0,i), g.stockVars[i-1]->name);
      CHECK_EQUAL(g.table.cell(2,i), g.flowVars[i-1]->name);
    }
}

/*
  a --
  b / \
        + -- c
*/
   

TEST_FIXTURE(TestFixture,multiVariableInputsAdd)
{
  VariablePtr varA = variables[variables.newVariable("a")];
  VariablePtr varB = variables[variables.newVariable("b")];
  VariablePtr varC = variables[variables.newVariable("c")];
  variables.values["a"]=0.1;
  variables.values["b"]=0.2;

  OperationPtr& intOp = operations[0]=OperationPtr(OperationType::integrate); //enables equations to step
  

  OperationPtr& op=operations[1]=OperationPtr(OperationType::add);

  addWire(Wire(varA->outPort(), op->ports()[1]));
  addWire(Wire(varB->outPort(), op->ports()[1]));
  addWire(Wire(op->ports()[0], varC->inPort()));
  addWire(Wire(varC->outPort(), intOp->ports()[1]));

  // move stuff around to make layout a bit better
  varA->MoveTo(10,100);
  varB->MoveTo(10,200);
  varC->MoveTo(100,150);
  op->MoveTo(50,150);
  intOp->MoveTo(150,150);

  Save("multiVariableInputs.mky");

  constructEquations();
  step();
  CHECK_CLOSE(0.3, variables.values["c"].value(), 1e-5);
}

TEST_FIXTURE(TestFixture,multiVariableInputsSubtract)
{
  VariablePtr varA = variables[variables.newVariable("a")];
  VariablePtr varB = variables[variables.newVariable("b")];
  VariablePtr varC = variables[variables.newVariable("c")];
  variables.values["a"]=0.1;
  variables.values["b"]=0.2;

  OperationPtr& intOp = operations[0]=OperationPtr(OperationType::integrate); //enables equations to step
  

  OperationPtr& op=operations[1]=OperationPtr(OperationType::subtract);

  addWire(Wire(varA->outPort(), op->ports()[2]));
  addWire(Wire(varB->outPort(), op->ports()[2]));
  addWire(Wire(op->ports()[0], varC->inPort()));
  addWire(Wire(varC->outPort(), intOp->ports()[1]));

  // move stuff around to make layout a bit better
  varA->MoveTo(10,100);
  varB->MoveTo(10,200);
  varC->MoveTo(100,150);
  op->MoveTo(50,150);
  intOp->MoveTo(150,150);

  Save("multiVariableInputs.mky");

  constructEquations();
  step();
  CHECK_CLOSE(-0.3, variables.values["c"].value(), 1e-5);
}

TEST_FIXTURE(TestFixture,multiVariableInputsMultiply)
{
  VariablePtr varA = variables[variables.newVariable("a")];
  VariablePtr varB = variables[variables.newVariable("b")];
  VariablePtr varC = variables[variables.newVariable("c")];
  variables.values["a"]=0.1;
  variables.values["b"]=0.2;

  OperationPtr& intOp = operations[0]=OperationPtr(OperationType::integrate); //enables equations to step
  

  OperationPtr& op=operations[1]=OperationPtr(OperationType::multiply);

  addWire(Wire(varA->outPort(), op->ports()[1]));
  addWire(Wire(varB->outPort(), op->ports()[1]));
  addWire(Wire(op->ports()[0], varC->inPort()));
  addWire(Wire(varC->outPort(), intOp->ports()[1]));

  // move stuff around to make layout a bit better
  varA->MoveTo(10,100);
  varB->MoveTo(10,200);
  varC->MoveTo(100,150);
  op->MoveTo(50,150);
  intOp->MoveTo(150,150);

  Save("multiVariableInputs.mky");

  constructEquations();
  step();
  CHECK_CLOSE(0.02, variables.values["c"].value(), 1e-5);
}

TEST_FIXTURE(TestFixture,multiVariableInputsDivide)
{
  VariablePtr varA = variables[variables.newVariable("a")];
  VariablePtr varB = variables[variables.newVariable("b")];
  VariablePtr varC = variables[variables.newVariable("c")];
  variables.values["a"]=0.1;
  variables.values["b"]=0.2;

  OperationPtr& intOp = operations[0]=OperationPtr(OperationType::integrate); //enables equations to step

  OperationPtr& op=operations[1]=OperationPtr(OperationType::divide);

  addWire(Wire(varA->outPort(), op->ports()[2]));
  addWire(Wire(varB->outPort(), op->ports()[2]));
  addWire(Wire(op->ports()[0], varC->inPort()));
  addWire(Wire(varC->outPort(), intOp->ports()[1]));

  // move stuff around to make layout a bit better
  varA->MoveTo(10,100);
  varB->MoveTo(10,200);
  varC->MoveTo(100,150);
  op->MoveTo(50,150);
  intOp->MoveTo(150,150);

  Save("multiVariableInputs.mky");

  constructEquations();
  step();
  CHECK_CLOSE(50, variables.values["c"].value(), 1e-5);
}
