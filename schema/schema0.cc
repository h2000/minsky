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
#include "schema/schema0.h"
#include "schema/schema0.cd"
#include "schemaHelper.h"
#include <ecolab_epilogue.h>
#include <fstream>
using namespace ecolab;
using namespace classdesc;
using namespace std;

namespace schema0
{
  using minsky::SchemaHelper;

  // like assignment between containers, but value_types needn't be identical
  template <class C1, class C2>
  void asg(C1& c1, const C2& c2)
  {
    for (typename C2::const_iterator i=c2.begin(); i!=c2.end(); ++i)
      c1.push_back(typename C1::value_type(*i));
  }

  template <class K, class V, class M>
  void asg(std::map<K,V>& m1, const M& m2)
  {
    for (typename M::const_iterator i=m2.begin(); i!=m2.end(); ++i)
      m1.insert(std::pair<K,V>(i->first,i->second));
  }
 
  template <>
  void asg(minsky::GroupIcons& m1, const Minsky::GroupIcons& m2)
  {
    for (Minsky::GroupIcons::const_iterator i=m2.begin(); i!=m2.end(); ++i)
      m1.insert(std::pair<int,minsky::GroupIcon>
                (i->first,minsky::GroupIcon(i->first))).first->second=i->second;
  }
 

  Operation::operator minsky::OperationPtr() const 
  {
    minsky::OperationPtr op(m_type, m_ports);
    op->m_x=x;
    op->m_y=y;
    // handle a previous schema change
    string desc=m_description.empty()? description: m_description;
    switch (m_type)
      {
      case OperationType::constant:
        if (minsky::Constant* c=dynamic_cast<minsky::Constant*>(op.get()))
          {
            c->description=desc;
            c->value=value;
            c->sliderVisible=sliderVisible;
            c->sliderBoundsSet=sliderBoundsSet;
            c->sliderStepRel=sliderStepRel;
            c->sliderMin=sliderMin;
            c->sliderMax=sliderMax;
            c->sliderStep=sliderStep;
          }
        break;
      case OperationType::integrate:
        if (minsky::IntOp* i=dynamic_cast<minsky::IntOp*>(op.get()))
          minsky::SchemaHelper::setPrivates(*i, desc, intVar);
        break;
      case OperationType::multiply: case OperationType::divide: 
        SchemaHelper::makePortMultiWired(minsky::minsky().ports[m_ports[1]]);
        SchemaHelper::makePortMultiWired(minsky::minsky().ports[m_ports[2]]);
        break;
      }

    op->rotation=rotation;
    op->visible=visible;
    // deal with an older schema
    return op;
  }

  VariablePtr::operator minsky::VariablePtr() const
  {
    minsky::VariablePtr v(m_type, name);
    v->MoveTo(x,y);
    v->Init(init);
    v->rotation=rotation;
    v->visible=visible;
    minsky::SchemaHelper::setPrivates(*v, m_outPort, m_inPort);
    v->m_godley=m_godley;
    return v;
  }

  VariableValue::operator minsky::VariableValue() const
  {
    minsky::VariableValue v(m_type, init);
    v.godleyOverridden=godleyOverridden;
    return v;
  }

  VariableManager::operator minsky::VariableManager() const
  {
    minsky::VariableManager vm;
    vm.insert(begin(), end());
    minsky::SchemaHelper::setPrivates(vm, wiredVariables, portToVariable);
    asg(vm.values, values);
    return vm;
  }

  GodleyTable::operator minsky::GodleyTable() const
  {
    minsky::GodleyTable g;
    minsky::SchemaHelper::setPrivates(g, data, m_assetClass);
    g.doubleEntryCompliant=doubleEntryCompliant;
    g.title=title;
    return g;
  }

  GodleyIcon::operator minsky::GodleyIcon() const
  {
    minsky::GodleyIcon g;
//    g.adjustHoriz=adjustHoriz;
//    g.adjustVert=adjustVert;
    g.MoveTo(x, y);
    g.scale=scale;
    asg(g.flowVars, flowVars);
    asg(g.stockVars, stockVars);
    g.table=table;
    return g;
  }

  GroupIcon::operator minsky::GroupIcon() const
  {
    minsky::GroupIcon g;
    minsky::SchemaHelper::setPrivates
      (g, operations, variables, wires, vector<int>(), inVariables, outVariables);
    minsky::SchemaHelper::setXY(g, x, y);
    g.name=name;
    g.width=width;
    g.height=height;
    g.rotation=rotation;
    return g;
  }

  void GroupIcon::updateEdgeVariables(const VariableManager& vm)
  {
    inVariables.clear();
    outVariables.clear();
    // for each edge port, we need to determine the edgeVariable
    // associated with it, and whether it is an input or output edge
    // variable
    for (size_t p=0; p<m_ports.size(); ++p)
      {
        map<int,int>::const_iterator vid=vm.portToVariable.find(m_ports[p]);
        if (vid!=vm.portToVariable.end())
          {
            VariableManager::Variables::const_iterator v=vm.find(vid->second);
            if (v!=vm.end())
              {
                if (v->second.m_inPort==m_ports[p])
                  inVariables.push_back(v->first);
                else if (v->second.m_outPort==m_ports[p])
                  outVariables.push_back(v->first);
              }
          }
      }
  }


  PlotWidget::operator minsky::PlotWidget() const
  {
    minsky::PlotWidget pw;
    pw.nxTicks=nxTicks;
    pw.nyTicks=nyTicks;
    pw.fontScale=fontScale;
    pw.offx=offx;
    pw.offy=offy;
    pw.logx=logx;
    pw.logy=logy;
    pw.grid=grid;
    pw.leadingMarker=leadingMarker;
    pw.autoscale=autoscale;
    pw.plotType=plotType;
    pw.minx=minx;
    pw.maxx=maxx;
    pw.miny=miny;
    pw.maxy=maxy;
    pw.ports=ports;
    asg(pw.yvars,yvars);
    asg(pw.xvars,xvars);
    pw.xminVar=xminVar;
    pw.xmaxVar=xmaxVar;
    pw.yminVar=yminVar;
    pw.ymaxVar=ymaxVar;
    pw.images=images;
    pw.MoveTo(x,y);
    return pw;
  }

  Plots::operator minsky::Plots() const
  {
    minsky::Plots p;
    asg(p.plots,plots);
    return p;
  }

  Minsky::Minsky(): stepMin(0), stepMax(1), nSteps(1),
                    epsAbs(1e-3), epsRel(1e-2) {}

  Minsky::operator minsky::Minsky() const
  {
    minsky::Minsky m;
    // override default minsky object for this method
    minsky::LocalMinsky lm(m);
    
    asg(m.ports, ports);
    asg(m.wires, wires);
    asg(m.godleyItems, godleyItems);
    m.variables=variables;
    m.operations.insert(operations.begin(), operations.end());
    asg(m.groupItems, groupItems);
    m.plots=plots;
    m.stepMin=stepMin;
    m.stepMax=stepMax;
    m.nSteps=nSteps;
    m.epsAbs=epsAbs;
    m.epsRel=epsRel;
    return m;
  }



  void Minsky::load(const char* filename)
  {
    ifstream inf(filename);
    xml_unpack_t saveFile(inf);
    saveFile >> *this;

    // if a godley table is present, and no godley icon present, copy
    // into godleyItems, to support XML migration
    if (godleyItems.empty() && godley.rows()>2)
      {
        godleyItems[0].table=godley;
        godleyItems[0].x=godleyItems[0].y=10;
      }
    
    map<int, xml_conversions::GodleyIcon> gItems;
    xml_unpack(saveFile,"root.godleyItems", gItems);
    
    for (GodleyItems::iterator g=godleyItems.begin(); g!=godleyItems.end(); ++g)
      if (g->second.flowVars.empty() && g->second.stockVars.empty())
        {
          xml_conversions::GodleyIcon& gicon=gItems[g->first];
          GodleyIcon& gi=g->second;
          asg(gi.flowVars, gicon.flowVars);
          asg(gi.stockVars, gicon.stockVars);
        }
    
    for (size_t i=0; i<groupItems.size(); ++i)
      groupItems[i].updateEdgeVariables(variables);

  }

}
