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

#include <xsd_generate_base.h>
#include <vector>
#include <string>

namespace schema1
{
  using minsky::SchemaHelper;
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
    OperationType::Type type;
    double value;
    vector<int> ports;
    string name;
    int intVar;
    Operation(): type(OperationType::numOps), value(0) {}
    Operation(int id, const minsky::OperationBase& op); 
  };

  struct Variable: public Item
  {
    VariableType::Type type;
    double init;
    vector<int> ports;
    string name;
    Variable(): type(VariableType::undefined), init(0) {}
    Variable(int id, const minsky::VariablePtr& v): 
      Item(id), type(v->type()), init(v->Init()), ports(toVector(v->ports())), 
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
      Item(id), ports(g.ports()), name(g.name) {}
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
      name(g.table.title), data(g.table.getData()), 
      assetClasses(g.table._assetClass()) {}
  };

  template <class T> 
  void xml_pack_layout(xml_pack_t& x, const string& d, const T& a);

  struct Layout
  {
    int id;
    Layout(int id=-1): id(id) {}
    virtual ~Layout() {}
    virtual void xml_pack(xml_pack_t&, const string&) const=0;
  };

  /// represent objects whose layouts just have a position (ports,
  /// plots, godleyIcons)
  struct PositionLayout: public virtual Layout
  {
    float x,y;

    PositionLayout(): x(0), y(0) {}
    template <class T> PositionLayout(int id, const T& item): 
      Layout(id), x(SchemaHelper::x(item)), y(SchemaHelper::y(item)) {}
    void xml_pack(xml_pack_t& x, const string& d) const
    {xml_pack_layout(x,d,*this);}
  };

  /// represents items with a visibility attribute
  struct VisibilityLayout: public virtual Layout
  {
    bool visible;
    VisibilityLayout(): visible(true) {}
    template <class T> VisibilityLayout(int id, const T& item):
      Layout(id), visible(item.visible) {}
    VisibilityLayout(int id, const minsky::GroupIcon& g):
      Layout(id), visible(true) {}
    void xml_pack(xml_pack_t& x, const string& d) const
    {xml_pack_layout(x,d,*this);}
  };

  /// represents layouts of wires
  struct WireLayout: public virtual VisibilityLayout
  {
    vector<float> coords;

    WireLayout() {}
    WireLayout(int id, const minsky::Wire& wire): 
      Layout(id), VisibilityLayout(id, wire), coords(toVector(wire.coords)) {}
    void xml_pack(xml_pack_t& x, const string& d) const
    {xml_pack_layout(x,d,*this);}
  };

  /// represents layouts of objects like variables and operators
  struct ItemLayout: public virtual PositionLayout, 
                     public virtual VisibilityLayout
  {
    float rotation;

    ItemLayout() {}
    template <class T> ItemLayout(int id, const T& item): 
      Layout(id), PositionLayout(id, item), VisibilityLayout(id, item),
      rotation(item.rotation) {}
    void xml_pack(xml_pack_t& x, const string& d) const
    {xml_pack_layout(x,d,*this);}
 };

  /// group layouts also have a width & height
  struct GroupLayout: public virtual ItemLayout
  {
    float width, height;
    GroupLayout() {}
    GroupLayout(int id, const minsky::GroupIcon& g):
      Layout(id), PositionLayout(id,g), VisibilityLayout(id, g),
      ItemLayout(id, g), width(g.width), height(g.height) {}
    void xml_pack(xml_pack_t& x, const string& d) const
    {xml_pack_layout(x,d,*this);}
  };

  /// describes item with sliders - currently just constants
  struct SliderLayout: public virtual ItemLayout
  {
    bool sliderVisible, sliderBoundsSet, sliderStepRel;
    double sliderMin, sliderMax, sliderStep;
    SliderLayout() {}
    SliderLayout(int id, const minsky::Constant& item):
      Layout(id), PositionLayout(id,item), VisibilityLayout(id, item), 
      ItemLayout(id, item), sliderVisible(item.sliderVisible),
      sliderBoundsSet(item.sliderBoundsSet), sliderStepRel(item.sliderStepRel),
      sliderMin(item.sliderMin), sliderMax(item.sliderMax), 
      sliderStep(item.sliderStep) {}
    void xml_pack(xml_pack_t& x, const string& d) const
    {xml_pack_layout(x,d,*this);}
  };

  /// structure representing a union of all of the above Layout
  /// classes, for xml_unpack
  struct UnionLayout: public virtual SliderLayout, 
                      public virtual GroupLayout, public virtual WireLayout
  {
    // not used, but needed to resolve ambiguity
    void xml_pack(xml_pack_t&,const string&) const {} 
  };

  inline void xml_pack(xml_pack_t& x, const string& d, 
                       const shared_ptr<Layout>& a)
  {a->xml_pack(x,d);}

  /// unpack into a UnionLayout structure, so everything's at hand 
  inline void xml_unpack(xml_unpack_t& x, const string& d, shared_ptr<Layout>& a)
  {
    a.reset(new UnionLayout);
    ::xml_unpack(x, d, dynamic_cast<UnionLayout&>(*a));
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
    vector<shared_ptr<Layout> > layout;
    float zoomFactor;
    Minsky(): schemaVersion(-1), zoomFactor(1) {} // schemaVersion defined on read in
    Minsky(const minsky::Minsky& m);

    operator minsky::Minsky() const;
    
  };
}

namespace classdesc
{
  // we provide a specialisation here, to ensure our intended schema
  // is as a "vector of Layouts"
  template <> inline std::string typeName<shared_ptr<schema1::Layout> >() 
  {return "schema1::Layout";}

  // decalare the polymorphic base classes
//  template <> struct IsPoly<schema1::UnionLayout>: 
//    public PolyBaseIs<schema1::Layout> {};

//  template <> inline
//  void xsd_generate(xsd_generate_t& x, const string& d, 
//                    const shared_ptr<schema1::Layout>& a)
//  {xsd_generate(x,d,schema1::UnionLayout());}


  // we're not using pack/unpack, so disable unpack because the
  // abstract base class causes problems

  inline void unpack(unpack_t&,const string&,shared_ptr<schema1::Layout>&) {}
}

using schema1::xml_pack;
using schema1::xml_unpack;
using classdesc::unpack;
using classdesc::xsd_generate;

#include "schema1.cd"

namespace schema1
{
  template <class T> 
  void xml_pack_layout(xml_pack_t& x, const string& d, const T& a)
  {
    //    ::xml_pack(x,d+".type",typeName<T>());
    ::xml_pack(x,d,a);
  }
}

#endif
