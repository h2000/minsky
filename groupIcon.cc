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

  struct Rectangle
  {
    float x0, y0, x1, y1;
    Rectangle(): x0(0), y0(0), x1(0), y1(0) {}
    Rectangle(float x, float y, float xx, float yy): 
      x0(x), y0(y), x1(xx), y1(yy) 
    {
      if (x0>x1) std::swap(x0, x1);
      if (y0>y1) std::swap(y0, y1);
    }
    bool inside(float x, float y)
    {
      return x>=x0 && x<=x1 && y>=y0 && y<=y1;
    }
  };

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
          GroupIcon& g=minsky::minsky.groupItems[id];
          double angle=g.rotation * M_PI / 180.0;
          xScale=yScale=g.zoomFactor();

          // determine how big the group icon should be to allow
          // sufficient space around the side for the edge variables
          float leftMargin, rightMargin;
          g.margins(leftMargin, rightMargin);
          leftMargin*=xScale; rightMargin*=xScale;

          unsigned width=xScale*g.width, height=yScale*g.height;
          if (width!=cairoSurface->width() || height!=cairoSurface->height())
            resize(width,height);
          cairoSurface->clear();


          // draw default group icon
          CairoRenderer renderer(cairoSurface->surface());
          cairo_save(renderer.cairo());

          cairo_translate(renderer.cairo(), 0.5*width, 0.5*height);


          cairo_rotate(renderer.cairo(), angle);
          cairo_translate(renderer.cairo(), -0.5*width, -0.5*height);
              
          double scalex=double(width-leftMargin-rightMargin)/width;
          cairo_translate(renderer.cairo(), leftMargin, 0);
          cairo_scale(renderer.cairo(), scalex, 1);
          if (g.displayContents())
            {
              // draw a simple frame
              cairo_rectangle(renderer.cairo(),0,0,width,height);
              cairo_stroke(renderer.cairo());
            }
          else
            {
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
              double transparency=g.displayContents()? 0.5: 1;
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
      tclcmd cmd;
      GroupIcon& g=minsky::minsky.groupItems[id];
      g.updatePortLocation();
      DisableEventProcessing e;
      if (display)
        {
          vector<int>::const_iterator i=g.operations().begin(); 
          for (; i!=g.operations().end(); ++i)
            if (!itemExists("op",*i))
              {
                cmd<<"drawOperation"<<*i<<"\n";
                //                cmd|".wiring.canvas addtag groupitems"|id|" withtag op"|*i|"\n";
                OperationBase& op=*minsky::minsky.operations[*i];
                op.visible=true;
                op.zoomFactor=g.localZoom();
                if (op.type()==OperationType::constant)
                  cmd<<"drawSlider"<<*i<<op.x()<<op.y()<<"\n";
              }

         set<int> edgeVars=g.edgeSet();
         for (i=g.variables().begin(); i!=g.variables().end(); ++i)
           if (edgeVars.count(*i)==0 && !itemExists("var",*i))
             {
               cmd<<"newVar"<<*i<<"\n";
               cmd|".wiring.canvas addtag groupitems"|id|" withtag var"|*i|"\n";
               VariableBase& v=*minsky::minsky.variables[*i];
               v.visible=true;
               v.zoomFactor=g.localZoom();
             }
         for (i=g.wires().begin(); i!=g.wires().end(); ++i)
           {
             Wire& w=minsky::minsky.wires[*i];
             w.visible=true;
             cmd << "adjustWire"<<w.to<<"\n";
             cmd|".wiring.canvas addtag groupitems"|id|" withtag wire"|*i|"\n";
           }
        }
      else
        {
          vector<int>::const_iterator i=g.operations().begin(); 
          for (; i!=g.operations().end(); ++i)
            {
              cmd|".wiring.canvas delete op"|*i|"\n";
              cmd|".wiring.canvas delete slider"|*i|"\n";
              OperationBase& op=*minsky::minsky.operations[*i];
              op.m_x/=op.zoomFactor;
              op.m_y/=op.zoomFactor;
              op.zoomFactor=1;
              op.visible=false;
            }
          set<int> eVars=g.edgeSet();
          for (i=g.variables().begin(); i!=g.variables().end(); ++i)
            if (!eVars.count(*i))
              {
                cmd|".wiring.canvas delete var"|*i|"\n";
                VariableBase& v=*minsky::minsky.variables[*i];
                v.m_x/=v.zoomFactor;
                v.m_y/=v.zoomFactor;
                v.zoomFactor=1;
                v.visible=false;              
              }
          for (i=g.wires().begin(); i!=g.wires().end(); ++i)
            {
              cmd|".wiring.canvas delete wire"|*i|"\n";
              minsky::minsky.wires[*i].visible=false;
            }
        }
      cmd|".wiring.canvas lower groups; update\n";
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

GroupIcon::LocalMinsky::LocalMinsky(Minsky& m) {l_minsky=&m;}
GroupIcon::LocalMinsky::~LocalMinsky() {l_minsky=NULL;}

Minsky& GroupIcon::minsky()
{
  if (l_minsky)
    return *l_minsky;
  else
    return minsky::minsky;
}

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
  vector<int>::const_iterator i=inVariables.begin();
  for (; i!=inVariables.end(); ++i)
    r.push_back(minsky().variables[*i]->inPort());
  for (i=outVariables.begin(); i!=outVariables.end(); ++i)
    r.push_back(minsky().variables[*i]->outPort());
  return r;
}

vector<VariablePtr> GroupIcon::edgeVariables() const
{
  vector<VariablePtr> r;

  // nb various methods below assume that the input variables come
  // before the output ones.
  for (size_t i=0; i<inVariables.size(); ++i)
    {
      r.push_back(minsky().variables[inVariables[i]]);
      assert(r.back()->type()!=VariableType::undefined);
    }
  for (size_t i=0; i<outVariables.size(); ++i)
    {
      r.push_back(minsky().variables[outVariables[i]]);
      assert(r.back()->type()!=VariableType::undefined);
    }
  return r;
}

void GroupIcon::addEdgeVariable
(vector<int>& varVector, vector<Wire>& additionalWires, int port)
{
  VariablePtr v=minsky().variables.getVariableFromPort(port);
  if (v->type()!=VariableBase::undefined)
    varVector.push_back(minsky().variables.getIDFromVariable(v));
  else
    {
      varVector.push_back(minsky().variables.newVariable(str(varVector.size())));
      VariablePtr v=minsky().variables[varVector.back()];
      m_variables.push_back(varVector.back());
      v->group=id;
      v->visible=false;
      Port& p=portManager().ports[port];
      // insert variable into wire
      array<int> wires=portManager().WiresAttachedToPort(port);
      for (int w=0; w<wires.size(); ++w)
        {
          Wire& wire=portManager().wires[wires[w]];
          if (p.input)
            portManager().wires[wires[w]].to=v->inPort();
          else
            portManager().wires[wires[w]].from=v->outPort();
        }
      if (p.input)
        additionalWires.push_back(Wire(v->outPort(),port));
      else
        additionalWires.push_back(Wire(port,v->inPort()));
      additionalWires.back().visible=false;
    }
}


void GroupIcon::group(float x0, float y0, float x1, float y1)
{
  assert(id>=0);
  Rectangle bbox(x0,y0,x1,y1);
  m_x=0.5*(x0+x1); m_y=0.5*(y0+y1);

  // track ports so we don't duplicate them in the group i/o entries,
  // also register unwired ports as an io port
  set<int> wiredPorts, edgePorts; 
  vector<Wire> additionalWires;

  for (PortManager::Wires::iterator w=portManager().wires.begin();
       w!=portManager().wires.end(); ++w)
    {
      const Port& from=portManager().ports[w->second.from];
      const Port& to=portManager().ports[w->second.to];
      wiredPorts.insert(w->second.from);
      wiredPorts.insert(w->second.to);
      if (bbox.inside(from.x(), from.y()))
        {
          if (bbox.inside(to.x(), to.y()))
            {
              m_wires.push_back(w->first);
              w->second.visible=false;
              // translate coordinates to be relative
              w->second.coords=translateWireCoords(w->second.coords,-x(),-y());
            }
          else if (edgePorts.insert(w->second.from).second)
            addEdgeVariable(outVariables, additionalWires, w->second.from);
        }
      else if (bbox.inside(to.x(), to.y()) && 
               edgePorts.insert(w->second.to).second) 
        addEdgeVariable(inVariables, additionalWires, w->second.to);

    }

  for (Operations::iterator o=minsky().operations.begin(); 
       o!=minsky().operations.end(); ++o)
    if (bbox.inside(o->second->x(),o->second->y()))
      {
        m_operations.push_back(o->first);
        o->second->move(-x(), -y());
        o->second->visible=false;
        o->second->group=id;
        if (IntOp* i=dynamic_cast<IntOp*>(o->second.get()))
          if (i->coupled()) 
            {
              i->toggleCoupled();
              // now capture the extra wired that's just been inserted
              int wid=portManager().WiresAttachedToPort(i->ports()[0])[0];
              Wire& w=portManager().wires[wid];
              wiredPorts.insert(w.to);
              wiredPorts.insert(w.from);
              m_wires.push_back(wid);
              w.visible=false;
            }
        const vector<int>& ports=o->second->ports();
        for (vector<int>::const_iterator p=ports.begin(); p!=ports.end(); ++p)
          if (wiredPorts.insert(*p).second)
            if (p==ports.begin())
              addEdgeVariable(outVariables, additionalWires, *p);
            else 
              addEdgeVariable(inVariables, additionalWires, *p);
      }

  // track variables already added
  set<int> varsAlreadyAdded(m_variables.begin(), m_variables.end());
  VariableManager& vars=minsky().variables;
  for (VariableManager::iterator v=vars.begin(); 
       v!=vars.end(); ++v)
    if (varsAlreadyAdded.count(v->first)==0 && 
        bbox.inside(v->second->x(),v->second->y()))
      {
        m_variables.push_back(v->first);
            

        v->second->visible=false;
        // make variable coordinates relative
        v->second->move(-x(), -y());
        
        v->second->group=id;
            
        if (wiredPorts.insert(v->second->inPort()).second)
          addEdgeVariable(inVariables, additionalWires, v->second->inPort());
        if (wiredPorts.insert(v->second->outPort()).second)
          addEdgeVariable(inVariables, additionalWires, v->second->outPort());

      }
  // now add the additional wires to port manager
  for (size_t i=0; i<additionalWires.size(); ++i)
    m_wires.push_back(portManager().addWire(additionalWires[i]));

  // make width & height slightly smaller than contentBounds
  contentBounds(x0,y0,x1,y1);
  width=floor(0.95*abs(x1-x0)); height=floor(0.95*abs(y1-y0));
  computeDisplayZoom();
  updatePortLocation();

  // centre contents on icon
  float dx=iconCentre()-m_x;
  for (size_t i=0; i<m_operations.size(); ++i)
    minsky().operations[m_operations[i]]->move(dx,0);
  set<int> eVars=edgeSet();
  for (size_t i=0; i<m_variables.size(); ++i)
    if (!eVars.count(m_variables[i]))
      minsky().variables[m_variables[i]]->move(dx,0);
}

void GroupIcon::ungroup()
{
  // we must apply visibility to the wires first, as the call to
  // toggleCoupled in the operations section potentially deletes a
  // wire.
  for (size_t i=0; i<m_wires.size(); ++i)
    portManager().wires[m_wires[i]].visible=true;

  for (size_t i=0; i<m_operations.size(); ++i)
    {
      OperationBase& o=*minsky().operations[m_operations[i]];
      o.group=-1; // TODO, set to parent id
      o.move(x(), y());
      o.visible=true;
      // TODO: set to zoomfactor of parent group
      o.zoom(x(),y(),minsky().zoomFactor());
      if (IntOp* i=dynamic_cast<IntOp*>(&o))
        if (!i->coupled()) i->toggleCoupled();
    }
  VariableManager& vars=minsky().variables;
  for (size_t i=0; i<m_variables.size(); ++i)
    {
      VariableBase& v=*vars[m_variables[i]];
      // restore variable coordinates to their absolute values
      v.group=-1; // TODO, set to parent id
      v.move(x(), y());
      // TODO: set to zoomfactor of parent group
      v.zoom(x(),y(),minsky().zoomFactor());
      if (v.type()!=VariableType::integral) 
        v.visible=true;
    }

  m_operations.clear();
  m_variables.clear();
  m_wires.clear();
  inVariables.clear();
  outVariables.clear();
}

void GroupIcon::MoveTo(float x1, float y1)
{
  float dx=x1-m_x, dy=y1-m_y;
  m_x=x1; m_y=y1;
  for (vector<int>::const_iterator i=m_wires.begin(); 
       i!=m_wires.end(); ++i)
    {
      Wire& w=minsky().wires[*i];
      w.move(dx,dy);
    }
  //  (tclcmd() | ".wiring.canvas move groupitems"|id) << dx << dy<<"\n";
  
  // move contained items (if any) 

  /*
    TODO, callbacks to TCL interpreter is way to slow - figure out how
   notify Tk of coordinate changes
  */
  if (displayContents())
    {
      DisableEventProcessing e;
      tclcmd cmd;
      for (vector<int>::const_iterator i=m_operations.begin(); 
           i!=m_operations.end(); ++i)
        {
          OperationBase& op=*minsky().operations[*i];
          //          (cmd|"submitUpdateItemPos op"|*i)<<"op"<<*i<<"\n";
          (cmd|".wiring.canvas coords op"|*i)<<op.x()<<op.y()<<"\n";
        }
      for (vector<int>::const_iterator i=m_variables.begin(); 
           i!=m_variables.end(); ++i)
        {
          VariableBase& v=*minsky().variables[*i];
          //(cmd|"submitUpdateItemPos var"|*i)<<"var"<<*i<<"\n";
          (cmd|".wiring.canvas coords var"|*i)<<v.x()<<v.y()<<"\n";        
        }
      for (vector<int>::const_iterator i=m_wires.begin(); 
           i!=m_wires.end(); ++i)
        (cmd|".wiring.canvas coords wire"|*i)<<minsky().wires[*i].coords<<"\n";  
    }      
}

array<float> GroupIcon::updatePortLocation()
{
  array<float> r;
  int toIdx=1, fromIdx=1; // port counters
  float angle=rotation * M_PI / 180.0;
  float ca=cos(angle), sa=sin(angle);
  
  float leftMargin, rightMargin;
  margins(leftMargin,rightMargin);

  vector<VariablePtr> eVars=edgeVariables();
  for (vector<VariablePtr>::iterator v=eVars.begin(); v!=eVars.end(); ++v)
    {
      // calculate the unrotated offset from icon position
      float dx, dy; 
      //      const float portOffset=8;
      RenderVariable rv(*v, NULL, zoomFactor(), zoomFactor());
      if (v-eVars.begin()<inVariables.size())
        {
          dx= zoomFactor()*(-0.5*width+leftMargin-rv.width()-2);
          dy= zoomFactor()* 2*rv.height() * (toIdx>>1) * (toIdx&1? -1:1);
          (r<<=-0.5*width)<<=dy;
          toIdx++;
        }
      else
        {
          dx= zoomFactor()*(0.5*width-rightMargin+rv.width());
          dy= zoomFactor()*2*rv.height() * (fromIdx>>1) * (fromIdx&1? -1:1);
          (r<<=0.5*width)<<=dy;
          fromIdx++;
        }
        
      // calculate rotated port positions
      (*v)->MoveTo(dx*ca-dy*sa+x(), dx*sa+dy*ca+y());
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
  m_wires.resize(src.m_wires.size());
  inVariables.resize(src.inVariables.size());
  outVariables.resize(src.outVariables.size());

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
        if (v->type()==VariableType::integral && intVarMap.count(v->name))
          // remap the variable name
          v->name=intVarMap[v->name];
        portMap.addPorts(*minsky().variables[srcVar], *v);
      }
  
  // add corresponding wires
  for (int i=0; i<src.m_wires.size(); ++i)
    {
      Wire w=minsky().wires[src.m_wires[i]];
      w.from=portMap[w.from]; w.to=portMap[w.to];
      m_wires[i]=static_cast<PortManager&>(minsky()).addWire(w);
    }

  // add corresponding I/O variables
  for (int i=0; i<src.inVariables.size(); ++i)
    inVariables[i]=minsky().variables.getVariableIDFromPort
      (portMap[minsky().variables[src.inVariables[i]]->inPort()]);
  for (int i=0; i<src.outVariables.size(); ++i)
    outVariables[i]=minsky().variables.getVariableIDFromPort
      (portMap[minsky().variables[src.outVariables[i]]->inPort()]);
 
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
  vector<int>::const_iterator i=inVariables.begin();
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
  left=right=0;
  vector<int>::const_iterator i=inVariables.begin();
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
  ::zoom(m_x, xOrigin, factor);
  ::zoom(m_y, yOrigin, factor);
  m_zoomFactor*=factor;
  updatePortLocation(); // should force edge wire coordinates to update
}

float GroupIcon::computeDisplayZoom()
{
  float x0, x1, y0, y1, l, r;
  contentBounds(x0,y0,x1,y1);
  margins(l,r);
  displayZoom=1.1*max((x1-x0)/(width-l-r), (y1-y0)/height);
  // centre contents within icon
//  float dx=iconCentre()-0.5*(x0+x1), dy=y()-0.5*(y0+y1);
//  vector<int>::iterator i=m_operations.begin();
//  for (;i!=m_operations.end(); ++i)
//    {
//      Operations::iterator o=minsky().operations.find(*i);
//      if (o!=minsky().operations.end())
//        o->second->move(dx,dy);
//    }
//  for (i=m_variables.begin();i!=m_variables.end(); ++i)
//    {
//      assert(minsky().variables.count(*i));
//      minsky().variables[*i]->move(dx,dy);
//      assert(minsky().variables[*i]->type()!=VariableType::undefined);
//    }
  return displayZoom;
}

void GroupIcon::AddVariable(int varId)
{
  VariableManager::iterator i=minsky().variables.find(varId);
  if (i!=minsky().variables.end())
    if (i->second->group!=id)
      {
        m_variables.push_back(varId);
        float x=i->second->x(), y=i->second->y();
        i->second->group=id;
        i->second->visible=displayContents();
        i->second->MoveTo(x,y); // adjust to group relative coordinates
        // see if any attached wires should also be moved into the group
        array<int> wiresToCheck=portManager().WiresAttachedToPort(i->second->inPort());
        wiresToCheck+=portManager().WiresAttachedToPort(i->second->outPort());
        if (wiresToCheck.size()>0)
          {
            // first build list of contained ports
            set<int> containedPorts;
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
                 Wire& wire=portManager().wires[*w];
                 if (containedPorts.count(wire.from) && 
                     containedPorts.count(wire.to))
                   m_wires.push_back(*w);
               }
          }
      }
}

void GroupIcon::removeVariable(int id)
{
}

void GroupIcon::addOperator(int id)
{
}

void GroupIcon::removeOperator(int id)
{
}

namespace 
{
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
  float ca=cos(M_PI*angle/180), sa=(M_PI*angle/180);

  vector<int>::const_iterator i=m_operations.begin();
  for (; i!=m_operations.end(); ++i)
    {
      OperationBase& o=*minsky().operations[*i];
      o.rotation+=angle;
      ::rotate(o.m_x, o.m_y, ca, sa);
    }
  for (i=m_variables.begin(); i!=m_variables.end(); ++i)
    {
      VariableBase& v=*minsky().variables[*i];
      v.rotation+=angle;
      ::rotate(v.m_x, v.m_y, ca, sa);
    }
  // transform wire coordinates
  for (i=m_wires.begin(); i!=m_wires.end(); ++i)
    {
      Wire& w=portManager().wires[*i];
      for (int j=0; j<w.coords.size(); j+=2)
        ::rotate(w.coords[j], w.coords[j+1], x(), y(), ca, sa);
    }
  // transform external ports
  vector<int> externalPorts=ports();
  for (i=externalPorts.begin(); i!=externalPorts.end(); ++i)
    {
      Port& p=portManager().ports[*i];
      float px=p.x(), py=p.y();
      ::rotate(px,py,x(),y(),ca,sa);
      portManager().movePortTo(*i,px,py);
    }
}
