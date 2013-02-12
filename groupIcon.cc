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
#include "groupIcon.h"
#include "minsky.h"
#include "init.h"
#include "cairoItems.h"
#include "XGLItem.h"
#include "str.h"
#include <xgl/cairorenderer.h>
#include <xgl/xgl.h>
#include <cairo_base.h>
#include <plot.h>
#include <ecolab_epilogue.h>
using namespace ecolab::cairo;
using namespace ecolab;
using namespace std;
using namespace minsky;

#include <boost/geometry/geometry.hpp>
using namespace boost::geometry;

namespace minsky
{
  using std::set;
namespace
{
  // allow room for port icons on vertical sides 
  const int portOffset=10;

  // for debugging
  void printPortLoc(int i)
  {
    Port& p=portManager().ports[i];
    printf("port[%d] @ (%f,%f)\n",i,p.x(),p.y());
  }

  // returns true if x==y, within a certain relative constant
  bool near_eq(double x, double y)
  {
    const double epsilon=1e-10;
    return (abs(x)<epsilon && abs(y)<epsilon) || 
      abs(x)>=epsilon && abs((x-y)/x)<epsilon;
  }

  // return true if \a rect overlaps partly, or wholly \a poly
  bool includesPart(const Rectangle& rect, const Polygon& poly)
  {
    return covered_by(return_envelope<Rectangle>(poly), rect) || 
      intersects(rect, poly);
  }

  /// translate a wire coordinate list by (\a dx,\a dy)
  array<float> translateWireCoords(array<float> c, float dx, float dy)
  {
    assert(c.size()%2==0);
    c[2*pcoord(c.size()/2)]+=dx;
    c[2*pcoord(c.size()/2)+1]+=dy;
    return c;
  }

  struct GroupIconItem: public XGLItem
  {
    GroupIconItem(const char* x): XGLItem(x), displayContents(false) {}
    bool displayContents;
    void draw()
    {
      if (cairoSurface && id>=0)
        {
          GroupIcon& g=minsky::minsky().groupItems[id];
          double angle=g.rotation * M_PI / 180.0;
          xScale=yScale=g.zoomFactor;

          // determine how big the group icon should be to allow
          // sufficient space around the side for the edge variables
          float leftMargin, rightMargin;
          g.margins(leftMargin, rightMargin);
          leftMargin*=xScale; rightMargin*=xScale;

          unsigned width=xScale*g.width, height=yScale*g.height;
          // bitmap needs to be big enough to allow a rotated
          // icon to fit on the bitmap.
          float rotFactor=g.rotFactor();
          resize(rotFactor*width,rotFactor*height);
          cairoSurface->clear();


          // draw default group icon
          CairoRenderer renderer(cairoSurface->surface());
          cairo_save(renderer.cairo());

          cairo_translate(renderer.cairo(), 0.5*cairoSurface->width(), 0.5*cairoSurface->height());


          cairo_rotate(renderer.cairo(), angle);

          // display I/O region in grey
          cairo_save(renderer.cairo());
          cairo_set_source_rgba(renderer.cairo(),0,1,1,0.5);
          cairo_rectangle(renderer.cairo(),-0.5*width,-0.5*height,leftMargin,height);
          cairo_rectangle(renderer.cairo(),0.5*width-rightMargin,-0.5*height,rightMargin,height);
          cairo_fill(renderer.cairo());
          cairo_restore(renderer.cairo());

          cairo_translate(renderer.cairo(), -0.5*width+leftMargin, -0.5*height);

              
          double scalex=double(width-leftMargin-rightMargin)/width;
          //double scaley=double(height)/cairoSurface->height();
          cairo_scale(renderer.cairo(), scalex, 1);
          //if (g.displayContents())
            {
              // draw a simple frame 
              cairo_rectangle(renderer.cairo(),0,0,width,height);
              cairo_save(renderer.cairo());
              cairo_identity_matrix(renderer.cairo());
              cairo_set_line_width(renderer.cairo(),1);
              cairo_stroke(renderer.cairo());
              cairo_restore(renderer.cairo());
            }
          if (!g.displayContents())
            {
              // render below uses bitmap size to determine image
              // size, so we need to scale by 1/rotFactor to get it to
              // correctly fit in the bitmap
              cairo_scale(renderer.cairo(), 1/rotFactor, 1/rotFactor);
              // replace contents with a graphic image
              xgl drawing(renderer);
              drawing.load(xglRes.c_str());
              drawing.render();
            }
          cairo_restore(renderer.cairo());

          cairo_t* cairo=cairoSurface->cairo();
          cairo_identity_matrix(cairo);
          cairo_translate(cairo,0.5*cairoSurface->width(),0.5*cairoSurface->height());

          g.updatePortLocation();
          g.drawEdgeVariables(cairo,xScale,yScale);
          if (displayContents!=g.displayContents())
            {
              createOrDeleteContentItems(g.displayContents());
              displayContents=g.displayContents();
            }


          // display text label
          if (!g.name.empty())
            {
              cairo_save(cairo);
              initMatrix();
              cairo_select_font_face
                (cairo, "sans-serif", CAIRO_FONT_SLANT_ITALIC, 
                 CAIRO_FONT_WEIGHT_NORMAL);
              cairo_set_font_size(cairo,12);
              
              // extract the bounding box of the text
              cairo_text_extents_t bbox;
              cairo_text_extents(cairo,g.name.c_str(),&bbox);
              double w=0.5*bbox.width+2; 
              double h=0.5*bbox.height+5;
              double fm=std::fmod(g.rotation,360);

              // if rotation is in 1st or 3rd quadrant, rotate as
              // normal, otherwise flip the text so it reads L->R
              if (fm>-90 && fm<90 || fm>270 || fm<-270)
                cairo_rotate(cairo, angle);
              else
                cairo_rotate(cairo, angle+M_PI);
 
              // prepare a background for the text, partially obscuring graphic
              double transparency=g.displayContents()? 0.25: 1;
              cairo_set_source_rgba(cairo,0,1,1,0.5*transparency);
              cairo_rectangle(cairo,-w,-h,2*w,2*h);
              cairo_fill(cairo);

              // display text
              cairo_move_to(cairo,-w+1,h-4);
              cairo_set_source_rgba(cairo,0,0,0,transparency);
              cairo_show_text(cairo,g.name.c_str());
              cairo_restore(cairo);
            }

          cairoSurface->blit();
        }
    }

    bool itemExists(const string& item, int id)
    {
      tclcmd cmd;
      cmd << ".wiring.canvas find withtag "|item|id|"\n";
      return !cmd.result.empty();
    }

    // if \a display is true, ensure content items are visible on
    // canvas, if false, then delete content itemse
    void createOrDeleteContentItems(bool display)
    {
      DisableEventProcessing e;
      tclcmd cmd;
      GroupIcon& g=minsky::minsky().groupItems[id];
      g.updatePortLocation();
      if (display)
        {
          vector<int>::const_iterator i=g.operations().begin(); 
          for (; i!=g.operations().end(); ++i)
            if (!itemExists("op",*i))
              {
                OperationBase& op=*minsky::minsky().operations[*i];
                op.visible=true;
                op.zoom(g.x(), g.y(), g.localZoom()/op.zoomFactor);
                cmd<<"drawOperation"<<*i<<"\n";
                cmd|".wiring.canvas addtag groupitems"|id|" withtag op"|*i|"\n";
                if (op.type()==OperationType::constant)
                  {
                    cmd<<"drawSlider"<<*i<<op.x()<<op.y()<<"\n";
                    cmd|".wiring.canvas addtag groupitems"|id|
                      " withtag slider"|*i|"\n";
                  }
              }

         set<int> edgeVars=g.edgeSet();
         for (i=g.variables().begin(); i!=g.variables().end(); ++i)
           if (edgeVars.count(*i)==0 && !itemExists("var",*i))
             {
               VariableBase& v=*minsky::minsky().variables[*i];
               v.visible=true;
               v.zoom(g.x(), g.y(), g.localZoom()/v.zoomFactor);
               cmd<<"newVar"<<*i<<"\n";
               cmd|".wiring.canvas addtag groupitems"|id|" withtag var"|*i|"\n";
             }
         for (i=g.groups().begin(); i!=g.groups().end(); ++i)
           if (!itemExists("groupItem",*i))
           {
             GroupIcon& gg=minsky::minsky().groupItems[*i];
             gg.visible=true;
             gg.zoom(g.x(), g.y(), g.localZoom()/gg.zoomFactor);
             cmd<<"newGroupItem"<<*i<<"\n";
             cmd|".wiring.canvas addtag groupitems"|id|" withtag groupItem"|*i|"\n";
           }
         for (i=g.wires().begin(); i!=g.wires().end(); ++i)
           if (!itemExists("wire",*i))
           {
             Wire& w=minsky::minsky().wires[*i];
             w.visible=true;
             cmd << "adjustWire"<<w.to<<"\n";
             cmd|".wiring.canvas addtag groupitems"|id|" withtag wire"|*i|"\n";
           }
        }
      else
        {
          cmd|".wiring.canvas delete groupitems"|id|"\n";
          vector<int>::const_iterator i=g.operations().begin(); 
          for (; i!=g.operations().end(); ++i)
            {
              OperationBase& op=*minsky::minsky().operations[*i];
              op.m_x/=op.zoomFactor;
              op.m_y/=op.zoomFactor;
              op.zoomFactor=1;
              op.visible=false;
            }
          set<int> eVars=g.edgeSet();
          for (i=g.variables().begin(); i!=g.variables().end(); ++i)
            if (!eVars.count(*i))
              {
                VariableBase& v=*minsky::minsky().variables[*i];
                v.m_x/=v.zoomFactor;
                v.m_y/=v.zoomFactor;
                v.zoomFactor=1;
                v.visible=false;              
              }
          for (i=g.wires().begin(); i!=g.wires().end(); ++i)
            {
              minsky::minsky().wires[*i].visible=false;
            }
         for (i=g.groups().begin(); i!=g.groups().end(); ++i)
           {
             GroupIcon& g=minsky::minsky().groupItems[*i];
             g.zoom(g.x(), g.y(), 1/g.zoomFactor);
             g.visible=false;
           }
        }
      // lower this group icon below everybody else, and it parents, and so on
      for (int toLower=id; toLower!=-1; )
        {
          cmd|".wiring.canvas lower groupItem"|toLower|";";
          toLower=minsky::minsky().groupItems[toLower].parent();
        }          
      //cmd|"update\n";
      cmd|"\n";
    }
  };

  // we need some extra fields to handle the additional options
  struct TkXGLItem: public ImageItem
  {
    int id; // C++ object identifier
    char* xglRes; // name of the XGL file to display    
  };

  int creatProc(Tcl_Interp *interp, Tk_Canvas canvas, 
                Tk_Item *itemPtr, int objc,Tcl_Obj *CONST objv[])
  {
    TkXGLItem* tkXGLItem=(TkXGLItem*)(itemPtr);
    tkXGLItem->id=-1;
    tkXGLItem->xglRes=NULL;
    int r=createImage<GroupIconItem>(interp,canvas,itemPtr,objc,objv);
    if (r==TCL_OK)
      {
        GroupIconItem* xglItem=(GroupIconItem*)(tkXGLItem->cairoItem);
        if (xglItem) 
          {
            xglItem->xglRes = tkXGLItem->xglRes;
            xglItem->id = tkXGLItem->id;
            xglItem->draw();
            TkImageCode::ComputeImageBbox(canvas, tkXGLItem);
          }
      }
    return r;
  }

#ifdef __GNUC__
#pragma GCC push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
  // register GodleyItem with Tk for use in canvases.
  int registerItem()
  {
    static Tk_ItemType iconType = xglItemType();
    iconType.name="group";
    iconType.createProc=creatProc;
    Tk_CreateItemType(&iconType);
    return 0;
  }
#ifdef __GNUC__
#pragma GCC pop
#endif

  Minsky* l_minsky=NULL;
}

static int dum=(initVec().push_back(registerItem), 0);

void GroupIcon::deleteContents()
{
  //remove any displayed items
  tclcmd()|"if [winfo exists .wiring.canvas] {"
    ".wiring.canvas delete groupitems"|id|"}\n"; 
  // delete all contained objects
  for (vector<int>::const_iterator i=m_operations.begin(); 
       i!=m_operations.end(); ++i)
    minsky().operations.erase(*i);
  for (vector<int>::const_iterator i=m_variables.begin(); 
       i!=m_variables.end(); ++i)
    minsky().variables.erase(*i);
  for (vector<int>::const_iterator i=m_wires.begin(); 
       i!=m_wires.end(); ++i)
    minsky().wires.erase(*i);
}


std::vector<int> GroupIcon::ports() const
{
  vector<int> r;
  set<int>::const_iterator i=inVariables.begin();
  for (; i!=inVariables.end(); ++i)
    r.push_back(minsky().variables[*i]->inPort());
  for (i=outVariables.begin(); i!=outVariables.end(); ++i)
    r.push_back(minsky().variables[*i]->outPort());
  return r;
}

float GroupIcon::x() const
{
  if (parent()==-1)
    return m_x;
  else
    return m_x+minsky().groupItems[parent()].x();
}

float GroupIcon::y() const
{
  if (parent()==-1)
    return m_y;
  else
    return m_y+minsky().groupItems[parent()].y();
}

float GroupIcon::rotFactor() const
{
  float rotFactor;
  float ac=abs(cos(rotation*M_PI/180));
  static const float invSqrt2=1/sqrt(2);
  if (ac>=invSqrt2) 
    rotFactor=1.15/ac; //use |1/cos(angle)| as rotation factor
  else
    rotFactor=1.15/sqrt(1-ac*ac);//use |1/sin(angle)| as rotation factor
  return rotFactor;
}

vector<VariablePtr> GroupIcon::edgeVariables() const
{
  vector<VariablePtr> r;

  // nb various methods below assume that the input variables come
  // before the output ones.
  for (set<int>::iterator i=inVariables.begin(); i!=inVariables.end(); ++i)
    {
      r.push_back(minsky().variables[*i]);
      assert(r.back()->type()!=VariableType::undefined);
    }
  for (set<int>::iterator i=outVariables.begin(); i!=outVariables.end(); ++i)
    {
      r.push_back(minsky().variables[*i]);
      assert(r.back()->type()!=VariableType::undefined);
    }
  return r;
}

void GroupIcon::addEdgeVariable
(set<int>& varVector, set<string>& varNames, 
 vector<Wire>& additionalWires, int port)
{
  VariablePtr v=minsky().variables.getVariableFromPort(port);
  if (v->type()!=VariableBase::undefined)
    {
      int id=minsky().variables.getIDFromVariable(v);
      if ((v->group==1 || 
           // don't insert another group's edge variables
           minsky().groupItems[v->group].edgeSet().count(id)==0) &&
          varNames.insert(v->Name()).second) // only insert if name is unique
        {
          varVector.insert(id);
          VariableManager::iterator vi=minsky().variables.find(id);
          if (parent()>-1) minsky().groupItems[parent()].removeVariable(*vi);
          addVariable(*vi);
          return;
        }
    }
  // create a new edge variable
  static int nextVarName=0;
  string newName=str(nextVarName);
  while (minsky().variables.values.count(newName)>0)
    {
      ++nextVarName;
      newName=str(nextVarName);
    }
  assert(varNames.count(newName)==0);
  varNames.insert(newName);
  int newId=minsky().variables.newVariable(newName);
  varVector.insert(newId);
  VariablePtr newV=minsky().variables[newId];
  m_variables.push_back(newId);
  createdVars.push_back(newId);
  newV->group=id;
  newV->visible=false;
  Port& p=portManager().ports[port];
  // insert variable into wire
  array<int> wires=portManager().WiresAttachedToPort(port);
  for (int w=0; w<wires.size(); ++w)
    {
      Wire& wire=portManager().wires[wires[w]];
      if (p.input)
        portManager().wires[wires[w]].to=newV->inPort();
      else
        portManager().wires[wires[w]].from=newV->outPort();
    }
  if (p.input)
    additionalWires.push_back(Wire(newV->outPort(),port));
  else
    additionalWires.push_back(Wire(port,newV->inPort()));
  additionalWires.back().visible=false;
  additionalWires.back().group=id;
}

void GroupIcon::eliminateIOduplicates()
{
  set<int> vars;
  inVarNames.clear();
  for (set<int>::iterator i=inVariables.begin(); i!=inVariables.end(); ++i)
    if (inVarNames.insert(minsky::minsky().variables[*i]->Name()).second)
      vars.insert(*i);
  inVariables.swap(vars);

  outVarNames.clear();
  vars.clear();
  for (set<int>::iterator i=outVariables.begin(); i!=outVariables.end(); ++i)
    if (outVarNames.insert(minsky::minsky().variables[*i]->Name()).second)
      vars.insert(*i);
  outVariables.swap(vars);

  assert(inVariables.size()==inVarNames.size());
  assert(outVariables.size()==outVarNames.size());
}

void GroupIcon::createGroup(float x0, float y0, float x1, float y1)
{
  assert(id>=0);
  Rectangle bbox(Point(x0,y0), Point(x1,y1));
  correct(bbox); //correct for x0>x1 or y0>y1.

  // set width and height initially to lasso bounds
  width=abs(x1-x0); height=abs(y1-y0);

  // track ports so we don't duplicate them in the group i/o entries,
  // also register unwired ports as an io port
  set<int> wiredPorts, edgePorts; 
  vector<Wire> additionalWires;

  // determine if group should be reparented
  {
    InGroup ig;
    ig.initGroupList(minsky().groupItems, id);
    int pg=ig.ContainingGroup(x0,y0);
    if (pg==ig.ContainingGroup(x1,y1))
      m_parent=pg;
  }
  GroupIcon* parentGroup=NULL;
  if (parent()>=0) parentGroup=&minsky().groupItems[parent()];

  MoveTo(0.5*(x0+x1), 0.5*(y0+y1));

  // we use this to exclude I/O ports belonging to groups that aren't
  // internal to this grouping. TODO - extend this logic to variables
  // and operators.
  set<int> externalGroupPorts;
  for (GroupIcons::iterator g=minsky().groupItems.begin(); 
       g!=minsky().groupItems.end(); ++g)
    {
      GroupIcon& gi=g->second;
      if (gi.id!=id && gi.parent()==parent() && 
          includesPart(bbox, gi.geom()))
        {
          addGroup(*g);
          gi.visible=false;
          if (parentGroup) parentGroup->removeGroup(*g);
        }
      else
        {
          if (gi.id==id)
            if (parentGroup) parentGroup->addGroup(*g);
          const vector<int>& p=gi.ports();
          externalGroupPorts.insert(p.begin(), p.end());
        }
    }

  for (PortManager::Wires::iterator w=minsky().wires.begin();
       w!=minsky().wires.end(); ++w)
    if (w->second.group==parent())
      {
        const Port& from=minsky().ports[w->second.from];
        const Port& to=minsky().ports[w->second.to];
        wiredPorts.insert(w->second.from);
        wiredPorts.insert(w->second.to);
        if (covered_by(Point(from.x(), from.y()), bbox) && 
            externalGroupPorts.count(w->second.from)==0)
          {
            if (covered_by(Point(to.x(), to.y()), bbox) && 
                externalGroupPorts.count(w->second.to)==0)
              {
                m_wires.push_back(w->first);
                if (parentGroup)
                  parentGroup->m_wires.erase
                    (remove(parentGroup->m_wires.begin(), 
                            parentGroup->m_wires.end(), w->first), 
                     parentGroup->m_wires.end());
                w->second.visible=false;
                array<float> coords=w->second.Coords();
                w->second.group=id;
                // translate coordinates to be relative
                w->second.Coords(coords);
              }
            else if (edgePorts.insert(w->second.from).second)
              addEdgeVariable(outVariables, outVarNames, additionalWires, w->second.from);
          }
        else if (covered_by(Point(to.x(), to.y()), bbox) && 
                 externalGroupPorts.count(w->second.to)==0 && 
                 edgePorts.insert(w->second.to).second) 
          addEdgeVariable(inVariables, inVarNames, additionalWires, w->second.to);

      }

  // track variables already added
  set<int> varsAlreadyAdded(m_variables.begin(), m_variables.end());
  VariableManager& vars=minsky().variables;
  for (VariableManager::iterator v=vars.begin(); 
       v!=vars.end(); ++v)
    if (v->second->group==parent() && varsAlreadyAdded.count(v->first)==0 && 
        includesPart(bbox, RenderVariable(v->second).geom()))
      {
        if (parentGroup) parentGroup->removeVariable(*v);
        addVariable(*v);
        v->second->visible=false;
        // if any of the wires from this variable terminate outside,
        // then push onto the I/O list
        array<int> inWires=minsky().WiresAttachedToPort(v->second->inPort());
        for (array<int>::iterator w=inWires.begin(); w!=inWires.end(); ++w)
          if (minsky().wires[*w].group!=id)
            {
              if (inVarNames.insert(v->second->Name()).second)
                inVariables.insert(v->first);
              break;
            }
        array<int> outWires=minsky().WiresAttachedToPort(v->second->outPort());
        for (array<int>::iterator w=outWires.begin(); w!=outWires.end(); ++w)
          if (minsky().wires[*w].group!=id)
            {
              if (outVarNames.insert(v->second->Name()).second)
                outVariables.insert(v->first);
              break;
            }
      }

  for (Operations::iterator o=minsky().operations.begin(); 
       o!=minsky().operations.end(); ++o)
    if (o->second->group==parent() && 
        intersects(bbox, RenderOperation(o->second).geom()))
      {
        if (parentGroup) parentGroup->removeOperation(*o);
        addOperation(*o);
        o->second->visible=false;
        // If an integral variable is output variable, unbind the
        // integral operator for neatness
        if (IntOp* integ=dynamic_cast<IntOp*>(o->second.get()))
          if (outVarNames.count(integ->description()) && integ->coupled())
            {
              integ->toggleCoupled();
              // add created wire to group
              assert(minsky::minsky().WiresAttachedToPort(integ->ports()[0]).size()==1);
              m_wires.push_back
                (minsky::minsky().WiresAttachedToPort(integ->ports()[0])[0]);
              Wire& w=minsky::minsky().wires[m_wires.back()];
              w.visible=false;
              array<float> coords=w.Coords();
              w.group=id;
              // translate coordinates to be relative
              w.Coords(coords);
            }
      }

  // now add the additional wires to port manager
  for (size_t i=0; i<additionalWires.size(); ++i)
    m_wires.push_back(portManager().addWire(additionalWires[i]));

  // make width & height slightly smaller than contentBounds
  contentBounds(x0,y0,x1,y1);
  width=0.95*abs(x1-x0); height=0.95*abs(y1-y0);
  computeDisplayZoom();
  updatePortLocation();

  // centre contents on icon
  float dx=x()-0.5f*(x0+x1), dy=y()-0.5f*(y0+y1);
  for (size_t i=0; i<m_operations.size(); ++i)
    minsky().operations[m_operations[i]]->move(dx,dy);
  set<int> eVars=edgeSet();
  for (size_t i=0; i<m_variables.size(); ++i)
    if (!eVars.count(m_variables[i]))
      minsky().variables[m_variables[i]]->move(dx,dy);
  for (size_t i=0; i<m_groups.size(); ++i)
    minsky().groupItems[m_groups[i]].move(dx,dy);


}

void GroupIcon::ungroup()
{
  DisableEventProcessing e;
  if (parent()>-1)
    {
      GroupIcons::iterator parentGroup=minsky().groupItems.find(parent());
      GroupIcons::iterator thisGroup=minsky().groupItems.find(id);
      while (!m_operations.empty())
        {
          Operations::iterator o=minsky().operations.find(m_operations.front());
          if (o!=minsky().operations.end())
            {
              parentGroup->second.addOperation(*o);
              parentGroup->second.addAnyWires(o->second->ports());
              removeOperation(*o);
              removeAnyWires(o->second->ports());
            }
        }
      
      set<int> removedVars(createdVars.begin(), createdVars.end());
      while (!m_variables.empty())
        {
          VariableManager::iterator v=minsky().variables.find(m_variables.front());
          if (v!=minsky().variables.end())
            {
              if (removedVars.count(m_variables.front())==0)
                {
                  parentGroup->second.addVariable(*v);
                  parentGroup->second.addAnyWires(v->second->ports());
                }
              removeVariable(*v);
              removeAnyWires(v->second->ports());
            }
        }
      
      parentGroup->second.removeGroup(*thisGroup);
    }
  else
    { //ungrouping into global scope

      // we must apply visibility to the wires first, as the call to
      // toggleCoupled in the operations section potentially deletes a
      // wire.
      for (size_t i=0; i<m_wires.size(); ++i)
        {
          Wire& w=portManager().wires[m_wires[i]];
          w.visible=true;
          array<float> coords=w.Coords();
          w.group=-1;
          w.Coords(coords);
        }

      set<int> intVars;
      for (size_t i=0; i<m_operations.size(); ++i)
        {
          OperationBase& o=*minsky().operations[m_operations[i]];
          float x=o.x(), y=o.y();
          o.group=-1;
          o.MoveTo(x, y);
          o.visible=true;
          o.setZoom(minsky().zoomFactor());
          if (IntOp* i=dynamic_cast<IntOp*>(&o))
            {
              if (!i->coupled()) i->toggleCoupled();
              intVars.insert(i->intVarID()); // save for use in next loop
            }
        }
      VariableManager& vars=minsky().variables;
      for (size_t i=0; i<m_variables.size(); ++i)
        {
          VariableBase& v=*vars[m_variables[i]];
          // restore variable coordinates to their absolute values
          float x=v.x(), y=v.y();
          v.group=-1; 
          v.MoveTo(x, y);
          v.setZoom(minsky().zoomFactor());
          if (intVars.count(m_variables[i])==0) 
            v.visible=true;
        }

      for (size_t i=0; i<m_groups.size(); ++i)
        {
          GroupIcon& g=minsky().groupItems[m_groups[i]];
          float x=g.x(), y=g.y();
          g.m_parent=parent();
          g.MoveTo(x,y);
          g.visible = true;
          g.setZoom(minsky().zoomFactor());
        }
    }


  assert(minsky().groupItems.uniqueGroupMembership());

  m_operations.clear();
  m_variables.clear();
  m_wires.clear();
  m_groups.clear();
  inVariables.clear();
  outVariables.clear();
  inVarNames.clear();
  outVarNames.clear();

  // remove variables created during the createGroup() operation
  for (vector<int>::const_iterator i=createdVars.begin(); 
       i!=createdVars.end(); ++i)
    {
      // before deleting the variables, we need to join the wires
      VariablePtr v=minsky().variables[*i];
      array<int> incomingWires=minsky().WiresAttachedToPort(v->inPort());
      assert(incomingWires.size()<=1);
      if (incomingWires.size()>0)
        {
          array<int> outgoingWires=minsky().WiresAttachedToPort(v->outPort());
          Wire& inWire=minsky().wires[incomingWires[0]];
          for (size_t j=0; j<outgoingWires.size(); ++j)
            {
              Wire& w=minsky().wires[outgoingWires[j]];
              w.from=inWire.from;
              // remove coordinates of connecting variable
              w.Coords(inWire.Coords()[pcoord(inWire.Coords().size()-2)]<<
                       w.Coords()[pcoord(w.Coords().size()-2)+2]);
              w.visible=inWire.visible;
              w.group=inWire.group;
              assert(w.Coords().size()>=4);
            }
        }
      minsky().variables.erase(*i);
    }

  assert(minsky().groupItems.uniqueGroupMembership());

}

void GroupIcon::MoveTo(float x1, float y1)
{
  float dx=x1-x(), dy=y1-y();
  m_x+=dx; m_y+=dy;
  
  /*
    TODO, callbacks to TCL interpreter is way to slow - figure out how
   notify Tk of coordinate changes
  */
  if (displayContents())
    {
      DisableEventProcessing e;
      tclcmd cmd;
      cmd<<"llength [info commands .wiring.canvas]\n";
      if (cmd.result=="0") return; // not in GUI environment
      for (vector<int>::const_iterator i=m_operations.begin(); 
           i!=m_operations.end(); ++i)
        {
          OperationBase& op=*minsky().operations[*i];
          (cmd|".wiring.canvas coords op"|*i)<<op.x()<<op.y()<<"\n";
        }
      for (vector<int>::const_iterator i=m_variables.begin(); 
           i!=m_variables.end(); ++i)
        {
          VariableBase& v=*minsky().variables[*i];
          (cmd|".wiring.canvas coords var"|*i)<<v.x()<<v.y()<<"\n";        
        }
      for (vector<int>::const_iterator i=m_wires.begin(); 
           i!=m_wires.end(); ++i)
        (cmd|".wiring.canvas coords wire"|*i)<<minsky().wires[*i].Coords()<<"\n";       for (vector<int>::const_iterator i=m_groups.begin(); 
           i!=m_groups.end(); ++i)
        {
          GroupIcon& g=minsky().groupItems[*i];
          // force movement of canvas items contained within
          g.MoveTo(g.x(),g.y());
          (cmd|".wiring.canvas coords groupItem"|*i)<<g.x()<<g.y()<<"\n";        
        }
 
    }      
}

array<float> GroupIcon::updatePortLocation()
{
  array<float> r;
  int toIdx=1, fromIdx=1; // port counters
  minsky::Rotate rotate(rotation, x(), y());

  float leftMargin, rightMargin;
  margins(leftMargin,rightMargin);

  vector<VariablePtr> eVars=edgeVariables();
  for (vector<VariablePtr>::iterator v=eVars.begin(); v!=eVars.end(); ++v)
    {
      // calculate the unrotated offset from icon position
      float dx, dy; 
      RenderVariable rv(*v, NULL, zoomFactor, zoomFactor);
      if (v-eVars.begin()<inVariables.size())
        {
          dx= zoomFactor*(-0.5*width+leftMargin-rv.width()-2);
          dy= zoomFactor* 2*rv.height() * (toIdx>>1) * (toIdx&1? -1:1);
          (r<<=-0.5*width)<<=dy;
          toIdx++;
        }
      else
        {
          dx= zoomFactor*(0.5*width-rightMargin+rv.width());
          dy= zoomFactor*2*rv.height() * (fromIdx>>1) * (fromIdx&1? -1:1);
          (r<<=0.5*width)<<=dy;
          fromIdx++;
        }
      (*v)->rotation=rotation;
      // calculate rotated port positions
      (*v)->MoveTo(rotate.x(x()+dx,y()+dy), rotate.y(x()+dx,y()+dy));
      assert((*v)->type()!=VariableType::undefined);
      rv.updatePortLocs(); // ensures ports locations are corrected for curren zoom
    }
  return r;
}

struct PortMap: public map<int, int>
{
  // add mapping from all src ports to dest ports
  template <class T>
  void addPorts(const T& src, const T& dest)
  {
    assert(src.numPorts()==dest.numPorts());
    for (size_t i=0; i<src.numPorts(); ++i)
      insert(make_pair(src.ports()[i],dest.ports()[i]));
  }
};

void GroupIcon::copy(const GroupIcon& src)
{
  //  a map of port correspondences
  PortMap portMap;
  int newId=id; //save id, to restore it after copy
  *this=src;
  id=newId;
  m_operations.resize(src.m_operations.size());
  m_variables.resize(src.m_variables.size());
  m_wires.clear();
  inVariables.clear();
  outVariables.clear();
  // TODO revisit this when scoped variables are introduced
  inVarNames=src.inVarNames;
  outVarNames=src.outVarNames;

  // map of integral variable names
  map<string,string> intVarMap;
  // set of "bound" integration variable - do not copy these
  set<int> integrationVars;

  // generate copies of operations
  for (int i=0; i<src.m_operations.size(); ++i)
    {
      m_operations[i]=minsky().CopyOperation(src.m_operations[i]);
      Operations& op=minsky().operations;
      OperationBase& srcOp=*op[src.m_operations[i]];
      OperationBase& destOp=*op[m_operations[i]];
      destOp.group=id;
      portMap.addPorts(srcOp, destOp);
      // add intVarMap entry if an integral
      if (IntOp* i=dynamic_cast<IntOp*>(&srcOp))
        {
          intVarMap[i->description()] = 
            dynamic_cast<IntOp&>(destOp).description();
          integrationVars.insert(i->intVarID());
        }
    }
  // generate copies of variables
  for (int i=0; i<src.m_variables.size(); ++i)
    if (!integrationVars.count(src.m_variables[i]))
      {
        int srcVar= src.m_variables[i];
        m_variables[i] = minsky().CopyVariable(srcVar);
        VariablePtr& v=minsky().variables[m_variables[i]];
        v->visible = minsky().variables[src.m_variables[i]]->visible;
        v->group = id;
        if (v->type()==VariableType::integral && intVarMap.count(v->Name()))
          // remap the variable name
          v->Name(intVarMap[v->Name()]);
        portMap.addPorts(*minsky().variables[srcVar], *v);
      }
  
  // add corresponding wires
  for (int i=0; i<src.m_wires.size(); ++i)
    {
      Wire w=minsky().wires[src.m_wires[i]];
      // if the wire connects to a variable, skip adding the wire (ticket #192)
      if (minsky().variables.getVariableIDFromPort(w.to)==-1)
        {
          w.from=portMap[w.from]; w.to=portMap[w.to];
          w.group=id;
          m_wires.push_back(static_cast<PortManager&>(minsky()).addWire(w));
        }
    }

  // add corresponding I/O variables
  for (set<int>::iterator i=src.inVariables.begin(); 
       i!=src.inVariables.end(); ++i)
    inVariables.insert(minsky().variables.getVariableIDFromPort
                       (portMap[minsky().variables[*i]->
                                inPort()]));
  for (set<int>::iterator i=src.outVariables.begin(); 
       i!=src.outVariables.end(); ++i)
    outVariables.insert(minsky().variables.getVariableIDFromPort
                        (portMap[minsky().variables[*i]->
                                 inPort()]));
 
  assert(inVariables.size()==inVarNames.size());
  assert(outVariables.size()==outVarNames.size());
}


void GroupIcon::contentBounds(float& x0, float& y0, float& x1, float& y1) const
{
  x0=x1=x();
  y0=y1=y();
  vector<int>::const_iterator i=m_operations.begin();
  for (; i!=m_operations.end(); ++i)
    {
      OperationPtr& op=minsky().operations[*i];
      assert(op);
      RenderOperation ro(op);
      x0=min(x0, op->x() - ro.width()*op->zoomFactor);
      x1=max(x1, op->x() + ro.width()*op->zoomFactor);
      y0=min(y0, op->y() - ro.height()*op->zoomFactor);
      y1=max(y1, op->y() + ro.height()*op->zoomFactor);
    }
  set<int> edgeVars=edgeSet();
  for (i=m_variables.begin(); i!=m_variables.end(); ++i)
    // exclude the edge variables from content bound calc    
    if (edgeVars.count(*i)==0) 
      {
        VariablePtr& v=minsky().variables[*i];
        assert(v);
        RenderVariable rv(v);
        x0=min(x0, v->x() - rv.width());
        x1=max(x1, v->x() + rv.width());
        y0=min(y0, v->y() - rv.height());
        y1=max(y1, v->y() + rv.height());
      }

  for (i=m_groups.begin(); i!=m_groups.end(); ++i)
    {
      GroupIcon& g=minsky().groupItems[*i];
      float w=0.5f*g.width*g.zoomFactor,
        h=0.5f*g.height*g.zoomFactor;
      x0=min(x0, g.x() - w);
      x1=max(x1, g.x() + w);
      y0=min(y0, g.y() - h);
      y1=max(y1, g.y() + h);
    }
  
}

void GroupIcon::drawVar
(cairo_t* cairo, const VariablePtr& v, float xScale, float yScale) const
{
  cairo_save(cairo);
  cairo_translate(cairo,v->x()-x(),v->y()-y());
  cairo_scale(cairo,xScale,yScale);
  RenderVariable(v, cairo, xScale, yScale).draw();
  cairo_restore(cairo);
}


void GroupIcon::drawEdgeVariables
(cairo_t* cairo, float xScale, float yScale) const
{
  set<int>::const_iterator i=inVariables.begin();
  for (; i!=inVariables.end(); ++i)
    {
      drawVar(cairo, minsky().variables[*i], xScale, yScale);
      assert(minsky().variables[*i]->type()!=VariableType::undefined);
    }
  for (i=outVariables.begin(); i!=outVariables.end(); ++i)
    {
      drawVar(cairo, minsky().variables[*i], xScale, yScale);
      assert(minsky().variables[*i]->type()!=VariableType::undefined);
    }
}

void GroupIcon::margins(float& left, float& right) const
{
  left=right=10;
  set<int>::const_iterator i=inVariables.begin();
  for (; i!=inVariables.end(); ++i)
    {
      assert(minsky().variables.count(*i));
      float w=2*RenderVariable(minsky().variables[*i]).width()+2;
      assert(minsky().variables[*i]->type()!=VariableType::undefined);
      if (w>left) left=w;
    }
  for (i=outVariables.begin(); i!=outVariables.end(); ++i)
    {
      assert(minsky().variables.count(*i));
      float w=2*RenderVariable(minsky().variables[*i]).width()+2;
      assert(minsky().variables[*i]->type()!=VariableType::undefined);
      if (w>right) right=w;
    }
}

void GroupIcon::zoom(float xOrigin, float yOrigin,float factor) {
  if (visible)
    {
      if (m_parent==-1)
        {
          ::zoom(m_x, xOrigin, factor);
          ::zoom(m_y, yOrigin, factor);
        }
      else
        {
          m_x*=factor;
          m_y*=factor;
        }
      zoomFactor*=factor;
      updatePortLocation(); // should force edge wire coordinates to update
    }
}

float GroupIcon::computeDisplayZoom()
{
  float x0, x1, y0, y1, l, r;
  contentBounds(x0,y0,x1,y1);
  x0=min(x0,x());
  x1=max(x1,x());
  y0=min(y0,y());
  y1=max(y1,y());
  margins(l,r);
  displayZoom=1.2*rotFactor()*max(max(
                          (x1-x())/(0.5f*width-r), 
                          (x()-x0)/(0.5f*width-l)),
                      max(
                          2*(y1-y())/height,
                          2*(y()-y0)/height));
  return displayZoom;
}

template <class S>
void GroupIcon::addAnyWires(const S& ports)
{
  array<int> wiresToCheck;
  for (size_t i=0; i<ports.size(); ++i)
    wiresToCheck <<= portManager().WiresAttachedToPort(ports[i]);
  if (wiresToCheck.size()>0)
    {
      // first build list of contained ports
      set<int> containedPorts;
      set<int> containedWires(m_wires.begin(), m_wires.end());
      for (size_t i=0; i<m_operations.size(); ++i)
        {
          const vector<int>& p=minsky().operations[m_operations[i]]->ports();
          containedPorts.insert(p.begin(), p.end());
        }
      for (size_t i=0; i<m_variables.size(); ++i)
        {
          const array<int>& p=minsky().variables[m_variables[i]]->ports();
          containedPorts.insert(p.begin(), p.end());
        }
      for (array<int>::iterator w=wiresToCheck.begin(); 
           w!=wiresToCheck.end(); ++w)
        {
          if (containedWires.count(*w)) continue; // already there, no
                                                  // need to add
          Wire& wire=portManager().wires[*w];
          if (containedPorts.count(wire.from) && 
              containedPorts.count(wire.to))
            {
              m_wires.push_back(*w);
              wire.group=id;
              wire.visible=displayContents();
            }
        }
    }
}

template void GroupIcon::addAnyWires(const array<int>& ports);
template void GroupIcon::addAnyWires(const vector<int>& ports);

template <class S>
void GroupIcon::removeAnyWires(const S& ports)
{
  set<int> portsToCheck(ports.begin(), ports.end());
  vector<int> newWires;
  GroupIcon* parentGroup=parent()>=0? &minsky().groupItems[parent()]: NULL;
  for (vector<int>::iterator i=m_wires.begin(); i!=m_wires.end(); ++i)
    {
      Wire& w=minsky().wires[*i];
      if (!portsToCheck.count(w.from) && !portsToCheck.count(w.to))
        newWires.push_back(*i);
      else
        {
          w.group=parent();
          w.visible=parentGroup? parentGroup->displayContents(): true;
        } 
    }
  m_wires.swap(newWires);
}

template void GroupIcon::removeAnyWires(const array<int>& ports);
template void GroupIcon::removeAnyWires(const vector<int>& ports);

int GroupIcon::InIORegion(float x, float y) const
{
  float left, right;
  margins(left,right);
  float dx=(x-this->x())*cos(rotation*M_PI/180)-
    (y-this->y())*sin(rotation*M_PI/180);
  if (0.5*width-right<dx)
    return 2;
  else if (-0.5*width+left>dx)
    return 1;
  else
    return 0;
}


void GroupIcon::addVariable(std::pair<const int,VariablePtr>& pv)
{
  if (pv.second->group!=id)
      {
        m_variables.push_back(pv.first);
        float x=pv.second->x(), y=pv.second->y();
        pv.second->group=id;

        // determine if variable is to be added to the interface variable list
        pv.second->visible=false;
        switch (InIORegion(x,y))
          {
          case 2:
            if (outVarNames.insert(pv.second->Name()).second)
              outVariables.insert(pv.first);
            break;
          case 1:
            if (inVarNames.insert(pv.second->Name()).second)
              inVariables.insert(pv.first);
            break;
          case 0:
            pv.second->visible=displayContents();
            break;
          }
        pv.second->MoveTo(x,y); // adjust to group relative coordinates
        computeDisplayZoom();
        updatePortLocation();
        assert(inVariables.size()==inVarNames.size());
        assert(outVariables.size()==outVarNames.size());
      }
}

void GroupIcon::removeVariable(std::pair<const int,VariablePtr>& pv)
{
  for (vector<int>::iterator i=m_variables.begin(); i!=m_variables.end(); ++i)
    if (*i==pv.first)
      {
        m_variables.erase(i);
        if (parent()==-1) //rebase coordinates
          {
            float x=pv.second->x(), y=pv.second->y();
            pv.second->group=-1;
            pv.second->visible=true;
            pv.second->MoveTo(x,y); // adjust to group relative coordinates
          }
        computeDisplayZoom();
        break;
      }
}

void GroupIcon::addOperation(std::pair<const int,OperationPtr>& po)
{
  if (po.second->group!=id)
    {
      m_operations.push_back(po.first);
      float x=po.second->x(), y=po.second->y();
      po.second->group=id;
      po.second->MoveTo(x,y); // adjust to group relative coordinates
      computeDisplayZoom();
      po.second->visible=displayContents();
    }
}

void GroupIcon::removeOperation(std::pair<const int,OperationPtr>& po)
{
  for (vector<int>::iterator i=m_operations.begin(); i!=m_operations.end(); ++i)
    if (*i==po.first)
      {
        m_operations.erase(i);
        computeDisplayZoom();
        if (parent()==-1) //rebase coordinates
          {
            float x=po.second->x(), y=po.second->y();
            po.second->group=-1;
            po.second->visible=true;
            po.second->MoveTo(x,y); // adjust to group relative coordinates
          }
        computeDisplayZoom();
        break;
      }
}

bool GroupIcon::isAncestor(int gid) const
{
  for (const GroupIcon* g=this; g->parent()!=-1; 
       g=&minsky().groupItems[g->parent()])
    if (g->parent()==gid)
      return true;
  return false;
}

bool GroupIcon::addGroup(std::pair<const int,GroupIcon>& pg)
{
  // do not add to self, or any ancestor to prevent cycles!
  if (pg.first==id || isAncestor(pg.first)) return false; 
  m_groups.push_back(pg.first);
  float x=pg.second.x(), y=pg.second.y();
  pg.second.m_parent=id;
  pg.second.MoveTo(x,y);
  pg.second.zoom(pg.second.x(), pg.second.y(), 
                     localZoom()/pg.second.zoomFactor);
  computeDisplayZoom();
  pg.second.visible=displayContents();
  return true;
}

void GroupIcon::removeGroup(std::pair<const int,GroupIcon>& pg)
{
  m_groups.erase(remove(m_groups.begin(), m_groups.end(), pg.first), m_groups.end());
  computeDisplayZoom();
  if (parent()==-1)
    {
      float x=pg.second.x(), y=pg.second.y();
      pg.second.m_parent=-1;
      pg.second.MoveTo(x,y);
      pg.second.zoom(pg.second.x(), pg.second.y(), 
                        minsky().zoomFactor()/pg.second.zoomFactor);
      pg.second.visible=true;
    }
}

void GroupIcon::setZoom(float factor)
{
  zoomFactor=factor;
  float lzoom=localZoom();
  vector<int>::iterator i;
  for (i=m_operations.begin(); i!=m_operations.end(); ++i)
    minsky::minsky().operations[*i]->setZoom(lzoom);
  for (i=m_variables.begin(); i!=m_variables.end(); ++i)
    minsky::minsky().variables[*i]->setZoom(lzoom);
  for (i=m_groups.begin(); i!=m_groups.end(); ++i)
    minsky::minsky().groupItems[*i].setZoom(lzoom);
}

namespace 
{
  // TODO: replace this with the Rotate class.

  // transform (x,y) by rotating around (0,0)
  inline void rotate(float& x, float& y, float ca, float sa)
  {
    float x1=ca*x-sa*y, y1=sa*x+ca*y;
    x=x1; y=y1;
  }

  // transform (x,y) by rotating around the origin(ox,oy)
  inline void rotate(float& x, float& y, float ox, float oy, float ca, float sa)
  {
    x-=ox; y-=oy;
    rotate(x,y,ca,sa);
    x+=ox; y+=oy;
  }
}

void GroupIcon::Rotate(float angle)
{
  rotation+=angle;
  //  float ca=cos(M_PI*angle/180), sa=(M_PI*angle/180);
  ::Rotate rotate(angle, x(), y());

  vector<int>::const_iterator i=m_operations.begin();
  for (; i!=m_operations.end(); ++i)
    {
      OperationBase& o=*minsky().operations[*i];
      o.rotation+=angle;
      Point p=rotate(o.x(), o.y());
      o.MoveTo(p.x(), p.y());
    }
  for (i=m_variables.begin(); i!=m_variables.end(); ++i)
    {
      VariableBase& v=*minsky().variables[*i];
      v.rotation+=angle;
      Point p=rotate(v.x(), v.y());
      v.MoveTo(p.x(), p.y());
    }
  // transform wire coordinates
  for (i=m_wires.begin(); i!=m_wires.end(); ++i)
    {
      Wire& w=portManager().wires[*i];
      array<float> coords=w.Coords();
      for (int j=0; j<coords.size(); j+=2)
        {
          Point p=rotate(coords[j], coords[j+1]);
          coords[j]=p.x();
          coords[j+1]=p.y();
        }
      w.Coords(coords); 
    }
  // transform external ports
  vector<int> externalPorts=ports();
  for (i=externalPorts.begin(); i!=externalPorts.end(); ++i)
    {
      Port& p=portManager().ports[*i];
      Point pp=rotate(p.x(), p.y());
      portManager().movePortTo(*i,pp.x(),pp.y());
    }
}

int GroupIcon::Select(float x, float y) const
{
  float left, right;
  margins(left, right);
  set<int>::const_iterator i;
  float dx=x-this->x(), dy=y-this->y();
  minsky::rotate(dx,dy, cos(rotation*M_PI/180.0), sin(rotation*M_PI/180.0));
  if (dx < -0.5*width+left)
    // check input variables
    for (i=inVariables.begin(); i!=inVariables.end(); ++i)
      if (RenderVariable(*minsky().variables[*i]).inImage(x,y))
        return *i;
  if (dx > 0.5*width-right)
    // check output variables
    for (i=outVariables.begin(); i!=outVariables.end(); ++i)
      if (RenderVariable(*minsky().variables[*i]).inImage(x,y))
        return *i;
  return -1;
}

Polygon GroupIcon::geom() const
{
  ::Rotate rotate(rotation, x(), y());
  float dx=0.5*width*zoomFactor, dy=0.5*height*zoomFactor;
  Polygon r;
  r+= rotate(x()-dx, y()-dy), rotate(x()-dx, y()+dy), 
    rotate(x()+dx, y()+dy), rotate(x()+dx, y()-dy);
  correct(r);
  return r;
}


vector<int> GroupIcons::visibleGroups() const
{
  vector<int> r;
  for (const_iterator i=begin(); i!=end(); ++i)
    if (i->second.visible)
      r.push_back(i->first);
  return r;
}

namespace
{
  bool insert(set<int>& items, const vector<int>& itemList)
  {
    for (size_t i=0; i<itemList.size(); ++i)
      if (!items.insert(itemList[i]).second)
        return false;
    return true;
  }
}

bool GroupIcons::uniqueGroupMembership() const
{
  set<int> varItems, opItems, wireItems, groupIds;
  for (GroupIcons::const_iterator g=begin(); g!=end(); ++g)
    if (!::insert(varItems, g->second.variables()) ||
        !::insert(opItems, g->second.operations()) ||
        !::insert(wireItems, g->second.wires()) ||
        !::insert(groupIds, g->second.groups()))
      return false;
  return true;
}



}
