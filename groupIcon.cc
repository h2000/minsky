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

          unsigned width=xScale*g.width, height=yScale*g.height;
          if (width!=cairoSurface->width() || height!=cairoSurface->height())
            resize(width,height);
          cairoSurface->clear();


          // determine if group icon is big enough to display contents
          //          bool displayContents = g.displayContents();
//          {
//            float x0, x1, y0, y1;
//            g.contentBounds(x0, y0, x1, y1);
//            float ww=0.5*(width-(leftMargin+rightMargin)*xScale);
//            float xcentre=g.x() + 0.5*width - rightMargin - ww;
//            displayContents = 
//              1.1*abs(x1-g.iconCentre()) < ww && 
//              1.1*abs(x0-g.iconCentre()) < ww &&
//              1.1*abs(y1-g.y()) < 0.5*height && 1.1*abs(y0-g.y()) < 0.5*height;
//          }
            
          // draw default group icon
          CairoRenderer renderer(cairoSurface->surface());
          cairo_save(renderer.cairo());

          cairo_translate(renderer.cairo(), 0.5*width, 0.5*height);


          cairo_rotate(renderer.cairo(), angle);
          cairo_translate(renderer.cairo(), -0.5*width, -0.5*height);
              
          double scalex=double(width-(leftMargin+rightMargin)*xScale)/width;
          cairo_translate(renderer.cairo(), leftMargin*xScale, 0);
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

          // draw the ports
//          initMatrix();
//          cairo_rotate(cairo, angle);
//          array<float> pLoc=g.updatePortLocation();
//          assert(pLoc.size() == 2*g.numPorts());

          //          for (size_t i=0; i<g.numPorts(); ++i)
          //            {
          //             // adjust output ports by the portOffset
          //              drawTriangle(cairo, pLoc[2*i], pLoc[2*i+1], 
          //                           palette[i%paletteSz]);
          //            }

          //          initMatrix();

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
                OperationBase& op=*minsky::minsky.operations[*i];
                op.visible=true;
//                if (op.type()==OperationType::constant)
//                  cmd<<"drawSlider"<<*i<<op.x()<<op.y()<<"\n";
              }

         set<int> edgeVars=g.edgeSet();
         for (i=g.variables().begin(); i!=g.variables().end(); ++i)
           if (edgeVars.count(*i)==0 && !itemExists("var",*i))
             {
               minsky::minsky.variables[id]->visible=true;
               cmd<<"newVar"<<*i<<"\n";
             }
         for (i=g.wires().begin(); i!=g.wires().end(); ++i)
           {
             Wire& w=minsky::minsky.wires[*i];
             w.visible=true;
             cmd << "adjustWire"<<w.to<<"\n";
//             if (!itemExists("wire",*i))
//               cmd<<"newWire"<<"[createWire {"<<w.coords<<"}]"<<*i<<"\n";
           }
         // TODO: this is overkill!
         //         cmd<<"after 1000 updateCanvas\n";
        }
      else
        {
          vector<int>::const_iterator i=g.operations().begin(); 
          for (; i!=g.operations().end(); ++i)
            {
              cmd|".wiring.canvas delete op"|*i|"\n";
              OperationBase& op=*minsky::minsky.operations[*i];
              op.zoom(g.iconCentre(), g.y(), 1/op.zoomFactor);
              op.visible=false;
            }
          for (i=g.variables().begin(); i!=g.variables().end(); ++i)
            {
              cmd|".wiring.canvas delete var"|*i|"\n";
              minsky::minsky.variables[*i]->visible=false;
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

}

static int dum=(initVec().push_back(registerItem), 0);

std::vector<int> GroupIcon::ports() const
{
  vector<int> r;
  vector<int>::const_iterator i=inVariables.begin();
  for (; i!=inVariables.end(); ++i)
    r.push_back(variableManager()[*i]->inPort());
  for (i=outVariables.begin(); i!=outVariables.end(); ++i)
    r.push_back(variableManager()[*i]->outPort());
  return r;
}

vector<VariablePtr> GroupIcon::edgeVariables() const
{
  vector<VariablePtr> r;

  // nb various methods below assume that the input variables come
  // before the output ones.
  for (size_t i=0; i<inVariables.size(); ++i)
    r.push_back(variableManager()[inVariables[i]]);
  for (size_t i=0; i<outVariables.size(); ++i)
    r.push_back(variableManager()[outVariables[i]]);
  return r;
}

void GroupIcon::addEdgeVariable
(vector<int>& varVector, vector<Wire>& additionalWires, int port, int groupId)
{
  VariablePtr v=variableManager().getVariableFromPort(port);
  if (v->type()!=VariableBase::undefined)
    varVector.push_back(variableManager().getIDFromVariable(v));
  else
    {
      varVector.push_back(variableManager().newVariable(str(varVector.size())));
      VariablePtr v=variableManager()[varVector.back()];
      m_variables.push_back(varVector.back());
      v->group=groupId;
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


void GroupIcon::group(float x0, float y0, float x1, float y1, int groupId)
{
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
            addEdgeVariable(outVariables, additionalWires, w->second.from, groupId);
        }
      else if (bbox.inside(to.x(), to.y()) && 
               edgePorts.insert(w->second.to).second) 
        addEdgeVariable(inVariables, additionalWires, w->second.to, groupId);

    }

  for (Operations::iterator o=minsky::minsky.operations.begin(); 
       o!=minsky::minsky.operations.end(); ++o)
    if (bbox.inside(o->second->x(),o->second->y()))
      {
        m_operations.push_back(o->first);
        o->second->move(-x(), -y());
        o->second->visible=false;
        o->second->group=groupId;
        const vector<int>& ports=o->second->ports();
        for (vector<int>::const_iterator p=ports.begin(); p!=ports.end(); ++p)
          if (wiredPorts.insert(*p).second)
            if (p==ports.begin())
              addEdgeVariable(outVariables, additionalWires, *p, groupId);
            else 
              addEdgeVariable(inVariables, additionalWires, *p, groupId);
      }

  // track variables already added
  set<int> varsAlreadyAdded(m_variables.begin(), m_variables.end());
  VariableManager::Variables& vars=minsky::minsky.variables;
  for (VariableManager::Variables::iterator v=vars.begin(); 
       v!=vars.end(); ++v)
    if (varsAlreadyAdded.count(v->first)==0 && 
        bbox.inside(v->second->x(),v->second->y()))
      {
        m_variables.push_back(v->first);
            

        v->second->visible=false;
        // make variable coordinates relative
        v->second->move(-x(), -y());
        
        v->second->group=groupId;
            
        if (wiredPorts.insert(v->second->inPort()).second)
          addEdgeVariable(inVariables, additionalWires, v->second->inPort(), groupId);
        if (wiredPorts.insert(v->second->outPort()).second)
          addEdgeVariable(inVariables, additionalWires, v->second->outPort(), groupId);

      }
  // now add the additional wires to port manager
  for (size_t i=0; i<additionalWires.size(); ++i)
    m_wires.push_back(portManager().addWire(additionalWires[i]));

  // make width & height slightly smaller than contentBounds
  contentBounds(x0,y0,x1,y1);
  width=floor(0.95*abs(x1-x0)); height=floor(0.95*abs(y1-y0));
  computeDisplayZoom();
  updatePortLocation();
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
      OperationBase& o=*minsky::minsky.operations[m_operations[i]];
      o.group=-1; // TODO, set to parent id
      o.move(x(), y());
      o.visible=true;
      // TODO: set to zoomfactor of parent group
      o.zoom(x(),y(),minsky::minsky.zoomFactor());
      if (IntOp* i=dynamic_cast<IntOp*>(&o))
        if (!i->coupled()) i->toggleCoupled();
    }
  VariableManager::Variables& vars=minsky::minsky.variables;
  for (size_t i=0; i<m_variables.size(); ++i)
    {
      VariableBase& v=*vars[m_variables[i]];
      // restore variable coordinates to their absolute values
      v.group=-1; // TODO, set to parent id
      v.move(x(), y());
      // TODO: set to zoomfactor of parent group
      v.zoom(x(),y(),minsky::minsky.zoomFactor());
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
  float dx=x1-x(), dy=y1-y();
  m_x=x1; m_y=y1;
  vector<VariablePtr> eVars=edgeVariables();
  for (vector<VariablePtr>::iterator v=eVars.begin(); v!=eVars.end(); ++v)
    (*v)->move(dx,dy);
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
  *this=src;
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
      m_operations[i]=minsky::minsky.CopyOperation(src.m_operations[i]);
      Operations& op=minsky::minsky.operations;
      OperationBase& srcOp=*op[src.m_operations[i]];
      OperationBase& destOp=*op[m_operations[i]];
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
        m_variables[i] = minsky::minsky.CopyVariable(src.m_variables[i]);
        VariablePtr& v=variableManager()[m_variables[i]];
        if (v->type()==VariableType::integral && intVarMap.count(v->name))
          // remap the variable name
          v->name=intVarMap[v->name];
        portMap.addPorts(*variableManager()[src.m_variables[i]], *v);
      }
  
  // add corresponding wires
  for (int i=0; i<src.m_wires.size(); ++i)
    {
      Wire w=minsky::minsky.wires[src.m_wires[i]];
      w.from=portMap[w.from]; w.to=portMap[w.to];
      m_wires[i]=portManager().addWire(w);
    }

  // add corresponding I/O variables
  for (int i=0; i<src.inVariables.size(); ++i)
    inVariables[i]=variableManager().getVariableIDFromPort
      (portMap[variableManager()[src.inVariables[i]]->inPort()]);
  for (int i=0; i<src.outVariables.size(); ++i)
    outVariables[i]=variableManager().getVariableIDFromPort
      (portMap[variableManager()[src.outVariables[i]]->inPort()]);
 
}


void GroupIcon::contentBounds(float& x0, float& y0, float& x1, float& y1) const
{
  x0=x1=x();
  y0=y1=y();
  vector<int>::const_iterator i=m_operations.begin();
  for (; i!=m_operations.end(); ++i)
    {
      OperationPtr& op=minsky.operations[*i];
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
        VariablePtr& v=minsky.variables[*i];
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
    drawVar(cairo, variableManager()[*i], xScale, yScale);
  for (i=outVariables.begin(); i!=outVariables.end(); ++i)
    drawVar(cairo, variableManager()[*i], xScale, yScale);
}

void GroupIcon::margins(float& left, float& right) const
{
  left=right=0;
  vector<int>::const_iterator i=inVariables.begin();
  for (; i!=inVariables.end(); ++i)
    {
      float w=2*RenderVariable(variableManager()[*i]).width()+2;
      if (w>left) left=w;
    }
  for (i=outVariables.begin(); i!=outVariables.end(); ++i)
    {
      float w=2*RenderVariable(variableManager()[*i]).width()+2;
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
  float dx=iconCentre()-0.5*(x0+x1), dy=y()-0.5*(y0+y1);
  vector<int>::iterator i=m_operations.begin();
  for (;i!=m_operations.end(); ++i)
    minsky::minsky.operations[*i]->move(dx,dy);
  for (i=m_variables.begin();i!=m_variables.end(); ++i)
    minsky::minsky.variables[*i]->move(dx,dy);
  return displayZoom;
}
