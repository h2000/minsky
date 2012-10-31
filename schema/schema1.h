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

/**
   @file schema 1 is a defined and published Minsky schema. It is
expected to grow over time, deprecated attributes are also allowed,
but any renamed attributes require bumping the schema number.  
*/
#ifndef SCHEMA_1_H
#define SCHEMA_1_H

#include "minsky.h"
#include "schemaHelper.h"

#include <vector>
#include <string>

namespace schema1
{
  using namespace std;
  struct Item
  {
    int id;
    Item(int id=-1): id(id) {}
  };

  struct Port: public Item
  {
    bool input;
    Port(): input(false) {}
    Port(int id, const minsky::Port& p): Item(id), input(p.input) {}
  };

  struct Wire: public Item
  {
    int from, to;
    Wire(): from(-1), to(-1) {}
    Wire(int id, const minsky::Wire& w): Item(id), from(w.from), to(w.to) {}
  };

  struct Operation: public Item
  {
    minsky::Operation::Type type;
    double value;
    vector<int> ports;
    string name;
    int intVar;
    Operation(): type(minsky::Operation::numOps), value(0) {}
    Operation(int id, const minsky::Operation& op): 
      Item(id), type(op.type()), value(op.value), ports(op.ports()), 
      name(op.getDescription()), intVar(op.intVarID()) {}
  };

  struct Variable: public Item
  {
    VariableType::Type type;
    double init;
    vector<int> ports;
    string name;
    Variable(): type(VariableType::undefined), init(0) {}
    Variable(int id, const minsky::VariablePtr& v): 
      Item(id), type(v->type()), ports(toVector(v->ports())), 
      name(v->name) {}
  };

  struct Plot: public Item
  {
    vector<int> ports;
    Plot() {}
    Plot(int id, const minsky::PlotWidget& p): 
      Item(id), ports(toVector(p.ports)) {}
  };

  struct Group: public Item
  {
    vector<int> items;
    vector<int> ports;
    string name;
    Group() {}
    Group(int id, const minsky::GroupIcon& g): 
      Item(id), ports(toVector(g.ports())), name(g.name) {}
  };

  struct Godley: public Item
  {
    vector<int> ports;
    bool doubleEntryCompliant;
    string name;
    vector<vector<string> > data;
    vector<minsky::GodleyTable::AssetClass> assetClasses;
    Godley(): doubleEntryCompliant(true) {}
    Godley(int id, const minsky::GodleyIcon& g):
      Item(id), ports(toVector(g.ports())), 
      doubleEntryCompliant(g.table.doubleEntryCompliant),
      name(g.table.title), data(g.table.getData()) {}
  };

  struct Layout: public Item
  {
    float x,y;
    bool visible;
    float rotation;
    bool sliderVisible;
    double sliderMin, sliderMax, sliderStep;
    vector<float> coords;
    float width, height;

    // body of default ctor
    void init() {x=y=rotation=0; visible=true; sliderVisible=false; 
      sliderMin=sliderMax=sliderStep=0; width=height=0;}
    Layout() {init();}
    Layout(int id, const minsky::Port& p): Item(id) {init(); x=p.x; y=p.y;}
    Layout(int id, const minsky::Wire& w): Item(id) 
    {init(); coords=toVector(w.coords); visible=w.visible;}
    Layout(int id, const minsky::Operation& o): Item(id) {
      init(); x=o.x; y=o.y; rotation=o.rotation; visible=o.visible;
      sliderVisible=o.sliderVisible; sliderMin=o.sliderMin; 
      sliderMax=o.sliderMax; sliderStep=o.sliderStep;
    }
    Layout(int id, const minsky::VariablePtr& v): Item(id) {
      init(); x=v->x; y=v->y; rotation=v->rotation; visible=v->visible;
    }
    Layout(int id, const minsky::PlotWidget& p): Item(id) {
      init(); x=p.x; y=p.y;
    }
    Layout(int id, const minsky::GodleyIcon& g): Item(id) {
      init(); x=g.x; y=g.y;
    }
    Layout(int id, const minsky::GroupIcon& g): Item(id) {
      init(); x=g.x; y=g.y; width=g.width; height=g.height; rotation=g.rotation;
    }
  };

  struct RungeKutta
  {
    double stepMin, stepMax;
    int nSteps;
    double epsRel, epsAbs;
    RungeKutta() {}
    RungeKutta(const minsky::Minsky& m):
      stepMin(m.stepMin), stepMax(m.stepMax), nSteps(m.nSteps),
      epsRel(m.epsRel), epsAbs(m.epsAbs) {}
  };

  struct MinskyModel
  {
    vector<Port> ports;
    vector<Wire> wires;
    vector<Operation> operations;
    vector<Variable> variables;
    vector<Plot> plots;
    vector<Group> groups;
    vector<Godley> godleys;
    RungeKutta rungeKutta;

    /// checks that all items are uniquely identified.
    bool validate() const;
  };

  struct Minsky
  {
    static const int version=1;
    int schemaVersion;
    MinskyModel model;
    vector<Layout> layout;
    Minsky(): schemaVersion(-1) {} // schemaVersion defined on read in
    Minsky(const minsky::Minsky& m);

    operator minsky::Minsky() const;
    
  };
}

#include "schema1.cd"
#endif
