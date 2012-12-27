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
    GroupIcon::LocalMinsky lm;
    VariablePtr a,b,c;
    int ab,bc; // wire ids
    TestFixture(): lm(*this)
    {
      setPortManager(*this);
      // create 3 variables, wire them and add first two to a group,
      // leaving 3rd external
      a=variables[variables.newVariable("a")];
      b=variables[variables.newVariable("b")];
      c=variables[variables.newVariable("c")];
      a->MoveTo(100,100);
      b->MoveTo(200,100);
      c->MoveTo(300,100);
      ab=PortManager::addWire(Wire(a->outPort(),b->inPort()));
      bc=PortManager::addWire(Wire(b->outPort(),c->inPort()));
      GroupIcon& g=groupItems[0]=GroupIcon(0);      
      g.group(50,50,250,150);
      CHECK_EQUAL(2, g.variables().size());
      CHECK_EQUAL(1, g.wires().size());
      CHECK_EQUAL(0, a->group);
      CHECK(!a->visible);
      CHECK_EQUAL(0, b->group);
      CHECK(!b->visible);
      CHECK_EQUAL(-1, c->group);
      CHECK(c->visible);
      CHECK(!wires[ab].visible);
      CHECK(wires[bc].visible);
      CHECK_EQUAL(ab, g.wires()[0]); 
    }
    ~TestFixture() {setPortManager(minsky::minsky);}
  };
}

SUITE(Group)
{
  TEST_FIXTURE(TestFixture, AddVariable)
  {
    GroupIcon& g=groupItems[0];
    g.AddVariable(variables.getIDFromVariable(c));
    CHECK_EQUAL(3, g.variables().size());
    CHECK_EQUAL(0, c->group);
    CHECK_EQUAL(variables.getIDFromVariable(c), g.variables().back());
    CHECK_EQUAL(2, g.wires().size());
    CHECK_EQUAL(bc, g.wires().back());
    CHECK(!wires[ab].visible);
    CHECK(!wires[bc].visible);
    
    // now check removal
    g.RemoveVariable(variables.getIDFromVariable(a));
    CHECK_EQUAL(2, g.variables().size());
    CHECK_EQUAL(-1, a->group);
    CHECK_EQUAL(1, g.wires().size());
    CHECK_EQUAL(bc, g.wires()[0]);
    CHECK(wires[ab].visible);
    CHECK(!wires[bc].visible);
  }

  TEST_FIXTURE(TestFixture, AddOperation)
  {
    int addOpId=AddOperation("add");
    int timeOpId=AddOperation("time");
    CHECK(addOpId >= 0);
    CHECK(timeOpId >= 0);
    OperationPtr addOp=operations[addOpId];
    OperationPtr timeOp=operations[timeOpId];
    addOp->MoveTo(150,100);
    timeOp->MoveTo(250,100);


    int timeA=PortManager::addWire(Wire(timeOp->ports()[0], a->inPort()));
    int bAdd=PortManager::addWire(Wire(b->outPort(), addOp->ports()[1]));

    GroupIcon& g=groupItems[0];

    CHECK_EQUAL(1, g.wires().size());
    g.AddOperation(addOpId);
    CHECK_EQUAL(1, g.operations().size());
    CHECK_EQUAL(addOpId, g.operations().back());
    CHECK_EQUAL(2, g.wires().size());
    CHECK_EQUAL(bAdd,  g.wires().back());
    CHECK_EQUAL(2, g.variables().size());
    CHECK(!addOp->visible);
    CHECK(timeOp->visible);
    CHECK_EQUAL(0,addOp->group);
    CHECK_EQUAL(-1,timeOp->group);
    CHECK(!wires[bAdd].visible);
    CHECK(wires[timeA].visible);

    g.AddOperation(timeOpId);
    CHECK_EQUAL(2, g.operations().size());
    CHECK_EQUAL(timeOpId, g.operations().back());
    CHECK_EQUAL(3, g.wires().size());
    CHECK_EQUAL(timeA,  g.wires().back());
    CHECK_EQUAL(2, g.variables().size());
    CHECK(!addOp->visible);
    CHECK(!timeOp->visible);
    CHECK_EQUAL(0,addOp->group);
    CHECK_EQUAL(0,timeOp->group);
    CHECK(!wires[bAdd].visible);
    CHECK(!wires[timeA].visible);
   
    g.RemoveOperation(addOpId);
    CHECK_EQUAL(1, g.operations().size());
    CHECK_EQUAL(timeOpId, g.operations().back());
    CHECK_EQUAL(2, g.wires().size());
    CHECK_EQUAL(timeA,  g.wires().back());
    CHECK_EQUAL(2, g.variables().size());
    CHECK(addOp->visible);
    CHECK(!timeOp->visible);
    CHECK_EQUAL(-1,addOp->group);
    CHECK_EQUAL(0,timeOp->group);
    CHECK(wires[bAdd].visible);
    CHECK(!wires[timeA].visible);
  }
}
