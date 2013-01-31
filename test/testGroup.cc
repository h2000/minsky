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
  void pWireCoords()
  {
    cout << "------------------------------------"<<endl;
    for (PortManager::Wires::const_iterator w=minsky::minsky().wires.begin();
           w!=minsky::minsky().wires.end(); ++w)
      cout <<"Wire "<<w->first<<": "<<w->second.Coords()<<endl;
  }

  struct TestFixture: public Minsky
  {
    LocalMinsky lm;
    VariablePtr a,b,c;
    int ab,bc; // wire ids
    TestFixture(): lm(*this)
    {
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

      // add a couple of time operators, to ensure the group has finite size
      operations[AddOperation("time")]->MoveTo(100,75);
      operations[AddOperation("time")]->MoveTo(200,125);
      g.createGroup(50,50,250,150);
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
      CHECK(uniqueGroupMembership());
    }
    bool insert(set<int>& items, const vector<int>& itemList)
    {
      for (size_t i=0; i<itemList.size(); ++i)
        if (!items.insert(itemList[i]).second)
          return false;
      return true;
    }

    bool uniqueGroupMembership()
    {
      set<int> varItems, opItems, wireItems, groupIds;
      for (GroupIcons::const_iterator g=groupItems.begin();
           g!=groupItems.end(); ++g)
        if (!insert(varItems, g->second.variables()) ||
            !insert(opItems, g->second.operations()) ||
            !insert(wireItems, g->second.wires()) ||
            !insert(groupIds, g->second.groups()))
          return false;
      return true;
    }
        
  };
}

SUITE(Group)
{
  TEST_FIXTURE(TestFixture, AddVariable)
  {
    GroupIcon& g=groupItems[0];
    AddVariableToGroup(0, variables.getIDFromVariable(c));
    CHECK_EQUAL(3, g.variables().size());
    CHECK_EQUAL(0, c->group);
    CHECK_EQUAL(variables.getIDFromVariable(c), g.variables().back());
    CHECK_EQUAL(2, g.wires().size());
    CHECK_EQUAL(bc, g.wires().back());
    CHECK(!wires[ab].visible);
    CHECK(!wires[bc].visible);
    
    CHECK(uniqueGroupMembership());

    // now check removal
    RemoveVariableFromGroup(0, variables.getIDFromVariable(a));
    CHECK_EQUAL(2, g.variables().size());
    CHECK_EQUAL(-1, a->group);
    CHECK_EQUAL(1, g.wires().size());
    CHECK_EQUAL(bc, g.wires().back());
    CHECK(wires[ab].visible);
    CHECK(!wires[bc].visible);
    CHECK(uniqueGroupMembership());
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
    AddOperationToGroup(0, addOpId);
    CHECK_EQUAL(3, g.operations().size());
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

    CHECK(uniqueGroupMembership());

    AddOperationToGroup(0, timeOpId);
    CHECK_EQUAL(4, g.operations().size());
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
    CHECK(uniqueGroupMembership());
   
    RemoveOperationFromGroup(0, addOpId);
    CHECK_EQUAL(3, g.operations().size());
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
    CHECK(uniqueGroupMembership());
  }

  TEST_FIXTURE(TestFixture, AddGroup)
  {
    GroupIcon& g=groupItems[0];
    GroupIcon& g1=groupItems[1]=GroupIcon(1);
    g1.createGroup(250,50,350,150);
    AddGroupToGroup(0,1);
    CHECK(g1.visible==g.displayContents());
    CHECK_EQUAL(1,g.groups().size());
    CHECK_EQUAL(1,g.groups()[0]);
    CHECK_EQUAL(g.localZoom(), g1.zoomFactor);
    CHECK(uniqueGroupMembership());

    RemoveGroupFromGroup(0,1);
    CHECK(g1.visible);
    CHECK_EQUAL(0,g.groups().size());
    CHECK_EQUAL(zoomFactor(), g1.zoomFactor);
    CHECK(uniqueGroupMembership());
  }


  TEST_FIXTURE(TestFixture, NestedGroupUngroup)
  {
    GroupIcon& g=groupItems[0];
    GroupIcon& g1=groupItems[CopyGroup(0)];
    g1.MoveTo(300,300);
    GroupIcon& g2=groupItems[Group(100,0,450,400)];
    CHECK_EQUAL(1,g2.variables().size());
    CHECK_EQUAL(1,g2.numPorts());
    CHECK_EQUAL(0,g2.operations().size());
    CHECK_EQUAL(1,g2.groups().size());
    CHECK_EQUAL(1,g2.groups()[0]);
    CHECK_EQUAL(0,g2.wires().size()); //TODO - not sure what the number of wires should be
    CHECK(uniqueGroupMembership());

    
    // now ungroup g1
    Ungroup(1);
    CHECK_EQUAL(0,g2.groups().size());
    CHECK_EQUAL(3, g2.variables().size());
    CHECK_EQUAL(2,g2.operations().size());
    CHECK_EQUAL(1,g2.wires().size());
    CHECK(uniqueGroupMembership());

    // now try to regroup within g2
    g2.computeDisplayZoom();
    while (!g2.displayContents()) 
      {
        Zoom(g2.x(),g2.y(),1.1);
      }

    float x0=g2.x()-0.5*g2.width*g2.zoomFactor,
      x1=g2.x()+0.5*g2.width*g2.zoomFactor,
      y0=g2.y()-0.5*g2.height*g2.zoomFactor,
      y1=g2.y()+0.5*g2.height*g2.zoomFactor;
    float b0,b1,b2,b3;
    g2.contentBounds(b0,b1,b2,b3);
    CHECK(b0>=x0 && b1>=y0 && b2<=x1 && b3<=y1);

    Save("NestedGroupUngroup.mky");
    GroupIcon& g3=groupItems[Group(x0, y0, x1, y1)]; 
    //TODO: this doesn't work correctly, possibly because some items
    //are being misplaced.
    CHECK_EQUAL(3,g3.variables().size());
    CHECK_EQUAL(2,g3.operations().size());
    CHECK_EQUAL(1,g3.wires().size());
    CHECK_EQUAL(0,g2.variables().size());
    CHECK_EQUAL(0,g2.operations().size());
    CHECK_EQUAL(0,g2.wires().size());
    CHECK_EQUAL(1,g2.groups().size());
    CHECK(uniqueGroupMembership());
  }

  // test that the new IO variables created do not introduce
  // extraneous references
  TEST_FIXTURE(Minsky, NewIOVariables)
  {
    LocalMinsky lm(*this);
    operations[AddOperation("exp")]->MoveTo(100,100);
    operations[AddOperation("exp")]->MoveTo(200,100);
    operations[AddOperation("exp")]->MoveTo(300,100);
    operations[AddOperation("exp")]->MoveTo(100,200);
    operations[AddOperation("exp")]->MoveTo(200,200);
    operations[AddOperation("exp")]->MoveTo(300,200);
    // following assume ports are allocated starting at 0
    PortManager::addWire(Wire(0,3));
    PortManager::addWire(Wire(2,5));
    PortManager::addWire(Wire(6,9));
    PortManager::addWire(Wire(8,11));
    Group(150,50,250,150);
    Group(150,150,250,250);
    Save("NewIOVariables.mky");
    CHECK(variables.size()==variables.values.size());
  }

}
