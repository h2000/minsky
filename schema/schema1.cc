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

#include "schema1.h"
#include "schemaHelper.h"
#include <ecolab_epilogue.h>

namespace schema1
{
  using minsky::SchemaHelper;
  const int Minsky::version;


  namespace
  {
    // internal type used in MinskyModel::validate()
    struct Validate  {
      set<int> items;
      template <class T> bool check(const T& x) {
        for (typename T::const_iterator i=x.begin(); i!=x.end(); ++i)
          if (!items.insert(i->id).second) return false;
        return true;
      }
    };

    // internal type for associating layout data with model data
    class Combine
    {
      typedef map<int, UnionLayout> Layouts;
      Layouts layout;
      minsky::VariableManager& vm;
    public:
      Combine(const vector<shared_ptr<Layout> >& l, minsky::VariableManager& vm): vm(vm) {
        for (size_t i=0; i<l.size(); ++i)
          {
            const UnionLayout* ul=dynamic_cast<const UnionLayout*>(l[i].get());
            if (ul)
              layout.insert(make_pair(l[i]->id, *ul));
          }
      }

      /// combine model and layout data
      template <class T, class U>
      T combine(T&, const U&) const;

      template <class T, class U>
      T combine(const T& t, const U& u) const
      {T t1(t); return combine(t1,u);}

      /// populate the usual integer-based map from a vector of schema data
      template <class T, class U>
      void populate(map<int, T>& m, const vector<U>& v) const
      {
        for (typename vector<U>::const_iterator i=v.begin(); i!=v.end(); ++i)
          combine(m[i->id], *i);
      }

      /// populate the GroupItem  from a vector of schema data
      void populate
      (map<int, minsky::GroupIcon>& m, const vector<Group>& v) const;

      /// populate the variable manager from a vector of schema data
      void populate(minsky::VariableManager& vm, const vector<Variable>& v) const
      {
        for (vector<Variable>::const_iterator i=v.begin(); i!=v.end(); ++i)
          {
            vm.addVariable(combine(minsky::VariablePtr(i->type), *i), i->id);
            vm.setInit(i->name, i->init);
          }
      }

      /// populate the plots manager from a vector of schema data
      void populate(minsky::Plots& plots, const vector<Plot>& v) const;
    };

    template <>
    minsky::Port Combine::combine(minsky::Port& p, const Port& pp) const
    {
      Layouts::const_iterator l=layout.find(pp.id);
      if (l!=layout.end())
        p=minsky::Port(l->second.x, l->second.y, pp.input);
      return p;
    }

    template <>
    minsky::Wire Combine::combine(minsky::Wire& w, const Wire& w1) const
    {
      Layouts::const_iterator l=layout.find(w1.id);
      if (l!=layout.end())
        {
          w.from=w1.from;
          w.to=w1.to;
          w.visible=l->second.visible;
          w.coords=toArray(l->second.coords);
        }
      return w;
    }

    template <> minsky::OperationPtr 
    Combine::combine(minsky::OperationPtr& o, const Operation& o1) const
    {
      Layouts::const_iterator li=layout.find(o1.id);
      o=minsky::OperationPtr(o1.type, o1.ports);
      if (o1.type==OperationType::integrate)
        SchemaHelper::setPrivates
          (dynamic_cast<minsky::IntOp&>(*o), o1.name, o1.intVar);
      if (minsky::Constant* c=dynamic_cast<minsky::Constant*>(o.get()))
        {
          c->value=o1.value;
          c->description=o1.name;
        }
      if (li!=layout.end())
        {
          const UnionLayout& l=li->second;
          o->rotation=l.rotation;
          o->visible=l.visible;
          if (minsky::Constant* c=dynamic_cast<minsky::Constant*>(o.get()))
            {
              c->sliderVisible=l.sliderVisible;
              c->sliderBoundsSet=l.sliderBoundsSet;
              c->sliderStepRel=l.sliderStepRel;
              c->sliderMin=l.sliderMin;
              c->sliderMax=l.sliderMax;
              c->sliderStep=l.sliderStep;
            }
          o->m_x=l.x;
          o->m_y=l.y;
        }
      return o;
    }
   
    template <> minsky::VariablePtr 
    Combine::combine(minsky::VariablePtr& v, const Variable& v1) const
    {
      assert(v);
      Layouts::const_iterator li=layout.find(v1.id);
      if (li!=layout.end())
        {
          const UnionLayout& l=li->second;
          v->Init(v1.init);
          v->name=v1.name;
          v->rotation=l.rotation;
          v->visible=l.visible;
          int out=v1.ports.size()>0? v1.ports[0]: -1;
          int in=v1.ports.size()>1? v1.ports[1]: -1;
          SchemaHelper::setPrivates(*v, out, in);
          v->m_x=l.x;
          v->m_y=l.y;
        }
      return v;
    }

    template <> minsky::PlotWidget 
    Combine::combine(minsky::PlotWidget& p, const Plot& p1) const
    {
      Layouts::const_iterator li=layout.find(p1.id);
      if (li!=layout.end())
        {
          const UnionLayout& l=li->second;
          p.ports.resize(p1.ports.size());
          for (size_t i=0; i<p1.ports.size(); ++i)
            p.ports[i]=p1.ports[i];
          p.MoveTo(l.x, l.y);
        }
      return p;
    }

    template <> minsky::GroupIcon
    Combine::combine(minsky::GroupIcon& g, const Group& g1) const
    {
      Layouts::const_iterator li=layout.find(g1.id);
      if (li!=layout.end())
        {
          const UnionLayout& l=li->second;
          // installing these has to happen later, as at this point,
          // we just have a list of anonymous items, not what type
          // they are
//          SchemaHelper::setPrivates
//            (g, g1.operations, g1.variables, g1.wires, g1.ports);
          g.name=g1.name;
          // moveTo needs to called later as well
          minsky::SchemaHelper::setXY(g, l.x, l.y);
          g.width=l.width;
          g.height=l.height;
          g.rotation=l.rotation;
        }
      return g;
    }

    template <> minsky::GodleyIcon
    Combine::combine(minsky::GodleyIcon& g, const Godley& g1) const
    {
     Layouts::const_iterator li=layout.find(g1.id);
      if (li!=layout.end())
        {
          const UnionLayout& l=li->second;
          SchemaHelper::setPrivates(g.table, g1.data, g1.assetClasses);
          g.table.doubleEntryCompliant=g1.doubleEntryCompliant;
          g.table.title=g1.name;
          // add in variables from the port list. Duplications don't
          // matter, as update() will fix this.
          for (size_t i=0; i<g1.ports.size(); ++i)
            {
              minsky::VariablePtr v = vm.getVariableFromPort(g1.ports[i]);
              v->m_godley=true;
              switch (v->type())
                {
                case VariableType::flow:
                  g.flowVars.push_back(v);
                  break;
                case VariableType::stock:
                  g.stockVars.push_back(v);
                }
            }
          g.update();
          g.MoveTo(l.x, l.y);
        }
      return g;
    }
    
    void Combine::populate
    (map<int, minsky::GroupIcon>& m, const vector<Group>& v) const
    {
      for (typename vector<Group>::const_iterator i=v.begin(); i!=v.end(); ++i)
        {
          minsky::GroupIcon& g = m.insert
            (make_pair(i->id, minsky::GroupIcon(i->id))).first->second;
          combine(g, *i);
         }
    }
     
    void Combine::populate(minsky::Plots& plots, const vector<Plot>& v) const
    {
      for (vector<Plot>::const_iterator i=v.begin(); i!=v.end(); ++i)
        {
          minsky::PlotWidget pw;
          // need to set the image key, otherwise PlotItem::draw
          // zeros out the location
          pw.images.push_back(plots.nextPlotID());
          plots.plots.insert(make_pair(pw.images[0], combine(pw, *i)));
        }
    }

    struct ItemMap: public map<int, int>
    {
      void remap(vector<int>& x) {
        for (size_t i=0; i<x.size(); ++i) x[i]=(*this)[x[i]];
      }
   };

    template <class T> shared_ptr<Layout> layoutFactory(int id, const T&);

    template <> shared_ptr<Layout> layoutFactory
    (int id, const minsky::Port& p)
    {return shared_ptr<Layout>(new PositionLayout(id,p));}

    template <> shared_ptr<Layout> layoutFactory
    (int id, const minsky::Wire& w)
    {return shared_ptr<Layout>(new WireLayout(id,w));}

    template <> shared_ptr<Layout> layoutFactory
    (int id, const minsky::VariablePtr& v)
    {return shared_ptr<Layout>(new ItemLayout(id,*v));}

    template <> shared_ptr<Layout> layoutFactory
    (int id, const minsky::OperationBase& o)
    {
      if (const minsky::Constant* c=dynamic_cast<const minsky::Constant*>(&o))
        return shared_ptr<Layout>(new SliderLayout(id,*c));
      else
        return shared_ptr<Layout>(new ItemLayout(id,o));
    }

    template <> shared_ptr<Layout> layoutFactory
    (int id, const minsky::GodleyIcon& g)
    {return shared_ptr<Layout>(new PositionLayout(id,g));}

    template <> shared_ptr<Layout> layoutFactory
    (int id, const minsky::PlotWidget& p)
    {return shared_ptr<Layout>(new PositionLayout(id,p));}

    template <> shared_ptr<Layout> layoutFactory
    (int id, const minsky::GroupIcon& g)
    {return shared_ptr<Layout>(new GroupLayout(id,g));}
  }

  Operation::Operation(int id, const minsky::OperationBase& op): 
    Item(id), type(op.type()), value(0), ports(op.ports()), intVar(-1) 
  {
    if (const minsky::Constant* c=dynamic_cast<const minsky::Constant*>(&op))
      {
        name=c->description;
        value=c->value;
      }
    else if (const minsky::IntOp* i=dynamic_cast<const minsky::IntOp*>(&op))
      {
        name=i->getDescription();
        intVar=i->intVarID();
      }
  }

  bool MinskyModel::validate() const
  {
    // check that wire from/to labels refer to ports
    set<int> portIds;
    for (vector<Port>::const_iterator p=ports.begin(); p!=ports.end(); ++p)
      portIds.insert(p->id);
    for (vector<Wire>::const_iterator w=wires.begin(); w!=wires.end(); ++w)
      if (portIds.find(w->from)==portIds.end() || portIds.find(w->to)==portIds.end())
        return false;

    // check that ids are unique
    Validate v;
    return v.check(ports) && v.check(wires) && v.check(operations) && 
      v.check(variables) && v.check(groups) && v.check(godleys);
  }

  

  Minsky::operator minsky::Minsky() const
  {
    if (!model.validate())
      throw ecolab::error("inconsistent Minsky model");

    minsky::Minsky m;
    Combine c(layout, m.variables);
    // override default minsky object for this method
    minsky::GroupIcon::LocalMinsky lm(m);

    c.populate(m.ports, model.ports);
    c.populate(m.wires, model.wires);
    c.populate(m.operations, model.operations);
    c.populate(m.variables, model.variables);
    m.variables.makeConsistent();
    c.populate(m.godleyItems, model.godleys);
    c.populate(m.groupItems, model.groups);
    c.populate(m.plots, model.plots);

    m.setZoom(zoomFactor);

    // separate the group item list into ports, wires, operations and
    // variables. Then set the Minsky model group item list to these
    // appropriate sublists.
    for (vector<Group>::const_iterator g=model.groups.begin(); 
         g!=model.groups.end(); ++g)
      {
        vector<int> ports, wires, ops, vars;
        for (vector<int>::const_iterator item=g->items.begin(); 
             item!=g->items.end(); ++item)
          if (m.ports.count(*item))
            ports.push_back(*item);
          else if (m.wires.count(*item))
            wires.push_back(*item);
          else if (m.operations.count(*item))
            ops.push_back(*item);
          else if (m.variables.count(*item))
            vars.push_back(*item);
        minsky::GroupIcon& gi=m.groupItems[g->id];

        vector<int> inVars, outVars;
        for (size_t i=0; i<ports.size(); ++i)
          {
            int varId=m.variables.getVariableIDFromPort(ports[i]);
            if (varId>=0)
              if (m.ports[ports[i]].input)
                inVars.push_back(varId);
              else
                outVars.push_back(varId);
          }
        SchemaHelper::setPrivates(gi, ops, vars, wires, inVars, outVars);
        for (vector<int>::const_iterator o=ops.begin(); o!=ops.end(); ++o)
          m.operations[*o]->group=g->id;
        for (vector<int>::const_iterator v=vars.begin(); v!=vars.end(); ++v)
          m.variables[*v]->group=g->id;
//        float zoomRatio = 1/gi.computeDisplayZoom();
//        // compute zoom factors of contained variables etc, if visible
//        for (vector<int>::const_iterator o=ops.begin(); o!=ops.end(); ++o)
//          m.operations[*o]->setZoom(zoomRatio);
//      
//        set<int> eVars(gi.edgeSet());
//        for (vector<int>::const_iterator v=vars.begin(); v!=vars.end(); ++v)
//          if (eVars.count(*v)==0)
//            m.variables[*v]->setZoom(zoomRatio);;
        gi.computeDisplayZoom();
        gi.computeDisplayZoom();
      }

    m.stepMin=model.rungeKutta.stepMin; 
    m.stepMax=model.rungeKutta.stepMax; 
    m.nSteps=model.rungeKutta.nSteps;   
    m.epsAbs=model.rungeKutta.epsAbs;   
    m.epsRel=model.rungeKutta.epsRel;   
    
    return m;
  }

  Minsky::Minsky(const minsky::Minsky& m)
  {
    schemaVersion = version;
    zoomFactor=m.zoomFactor();

    int id=0; // we're renumbering items
    ItemMap portMap; // map old ports ids to the new ones
    ItemMap wireMap;
    ItemMap opMap;
    ItemMap varMap;
    
    for (minsky::Minsky::Ports::const_iterator p=m.ports.begin(); 
         p!=m.ports.end(); ++p, ++id)
      {
        model.ports.push_back(Port(id, p->second));
        layout.push_back(layoutFactory(id, p->second));
        portMap[p->first]=id;
      }

    for (minsky::Minsky::Wires::const_iterator w=m.wires.begin(); 
         w!=m.wires.end(); ++w, ++id)
      {
        model.wires.push_back(Wire(id, w->second));
        model.wires.back().from=portMap[w->second.from];
        model.wires.back().to=portMap[w->second.to];
        layout.push_back(layoutFactory(id, w->second));
        wireMap[w->first]=id;
      }

     for (minsky::VariableManager::const_iterator v=m.variables.begin(); 
         v!=m.variables.end(); ++v, ++id)
      {
        model.variables.push_back(Variable(id, v->second));
        Variable& var=model.variables.back();
        portMap.remap(var.ports);
        layout.push_back(layoutFactory(id, v->second));
        var.init=m.variables.values.find(var.name)->second.init;
        varMap[v->first]=id;
      }

     for (minsky::Operations::const_iterator o=m.operations.begin(); 
         o!=m.operations.end(); ++o, ++id)
      {
        model.operations.push_back(Operation(id, *o->second));
        portMap.remap(model.operations.back().ports);
        model.operations.back().intVar=varMap[model.operations.back().intVar];
        layout.push_back(layoutFactory(id, *o->second));
        opMap[o->first]=id;
      }

      for (minsky::Minsky::GodleyItems::const_iterator g=m.godleyItems.begin(); 
           g!=m.godleyItems.end(); ++g, ++id)
      {
        model.godleys.push_back(Godley(id, g->second));
        portMap.remap(model.godleys.back().ports);
        layout.push_back(layoutFactory(id, g->second));
      }

      for (minsky::Minsky::GroupIcons::const_iterator g=m.groupItems.begin(); 
           g!=m.groupItems.end(); ++g, ++id)
      {
        model.groups.push_back(Group(id, g->second));
        Group& group=model.groups.back();

        // add in reamapped items 
        group.items.clear(); // in case this was set during construction
        const minsky::GroupIcon& g1=g->second;
        for (size_t i=0; i<g1.ports().size(); ++i)
          group.items.push_back(portMap[g1.ports()[i]]);
        for (size_t i=0; i<g1.wires().size(); ++i)
          group.items.push_back(wireMap[g1.wires()[i]]);
        for (size_t i=0; i<g1.operations().size(); ++i)
          group.items.push_back(opMap[g1.operations()[i]]);
        for (size_t i=0; i<g1.variables().size(); ++i)
          group.items.push_back(varMap[g1.variables()[i]]);

        layout.push_back(layoutFactory(id, g->second));
      }

      for (minsky::Plots::Map::const_iterator p=m.plots.plots.begin(); 
           p!=m.plots.plots.end(); ++p, ++id)
      {
        model.plots.push_back(Plot(id, p->second));
        portMap.remap(model.plots.back().ports);        
        layout.push_back(layoutFactory(id, p->second));
      }

      model.rungeKutta=RungeKutta(m);
      
      relocateCanvas();
  
  }

  void Minsky::relocateCanvas()
  {
    // ignore group items, as these coordinates are relative to group centre
    set<int> groupItems;
    for (vector<Group>::iterator i=model.groups.begin(); 
         i<model.groups.end(); ++i)
      groupItems.insert(i->items.begin(), i->items.end());

    // compute min values of coordinates
    float xmin=numeric_limits<float>::max(), 
      ymin=numeric_limits<float>::max();
    for (size_t i=0; i<layout.size(); ++i)
      {
        if (groupItems.count(layout[i]->id)) continue;
        if (PositionLayout* p=dynamic_cast<PositionLayout*>(layout[i].get()))
          {
            if (p->x<xmin) xmin=p->x;
            if (p->y<ymin) ymin=p->y;
          }
        else if (WireLayout* w=dynamic_cast<WireLayout*>(layout[i].get()))
          {
            for (size_t i=0; i<w->coords.size(); i+=2)
              if (w->coords[i]<xmin) xmin=w->coords[i];
            for (size_t i=1; i<w->coords.size(); i+=2)
              if (w->coords[i]<ymin) ymin=w->coords[i];
          }
      }
    // move all items be (-xmin, -ymin)
    ymin-=zoomFactor*15; // additional margin in vertical direction
    for (size_t i=0; i<layout.size(); ++i)
      {
        if (groupItems.count(layout[i]->id)) continue;
        if (PositionLayout* p=dynamic_cast<PositionLayout*>(layout[i].get()))
          {
            p->x-=xmin;
            p->y-=ymin;
          }
        else if (WireLayout* w=dynamic_cast<WireLayout*>(layout[i].get()))
          {
            for (size_t i=0; i<w->coords.size(); i+=2)
              w->coords[i]-=xmin;
            for (size_t i=1; i<w->coords.size(); i+=2)
              w->coords[i]-=ymin;
          }
      }
  }

}

