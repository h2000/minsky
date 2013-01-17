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
#include "XGLItem.h"
#include "variable.h"
#include "portManager.h"
#include "godleyIcon.h"
#include "cairoItems.h"
#include "minsky.h"
#include "init.h"
#include <xgl/cairorenderer.h>
#include <xgl/xgl.h>
#include <arrays.h>
#include <cairo_base.h>
#include <ctype.h>
#include <ecolab_epilogue.h>
using namespace ecolab::cairo;
using namespace ecolab;
using namespace std;
using namespace minsky;

namespace
{
  struct OrderByName
  {
    bool operator()(const VariablePtr& x, const VariablePtr& y) const
    {assert(x&&y); return x->name<y->name;}
  };

  struct GodleyIconItem: public XGLItem
  {
    GodleyIconItem(const char* x): XGLItem(x) {}
    void draw()
    {
      if (cairoSurface && id>=0)
        {
          GodleyIcon& gIcon=minsky::minsky().godleyItems[id];

          if (gIcon.width()!=cairoSurface->width() || 
              gIcon.height()!=cairoSurface->height())
            resize(gIcon.width(), gIcon.height());

          cairoSurface->clear();
          cairo_t *cairo=cairoSurface->cairo();


          CairoRenderer renderer(cairoSurface->surface());
          cairo_translate(renderer.cairo(), gIcon.leftMargin(), 0);
          cairo_scale(renderer.cairo(), 
                      (gIcon.width()-gIcon.leftMargin())/gIcon.width(),
                      (gIcon.height()-gIcon.bottomMargin())/gIcon.height());
          xgl drawing(renderer);
          drawing.load(xglRes.c_str());
          drawing.render();

          if (!gIcon.table.title.empty())
            {
              cairo_save(cairo);
              initMatrix();
              cairo_move_to(cairo,0.5*gIcon.leftMargin(),-0.5*gIcon.bottomMargin());
              cairo_select_font_face
                (cairo, "sans-serif", CAIRO_FONT_SLANT_ITALIC, 
                 CAIRO_FONT_WEIGHT_NORMAL);
              cairo_set_font_size(cairo,12);
              cairo_set_source_rgb(cairo,0,0,0);

              cairo_text_extents_t bbox;
              cairo_text_extents(cairo,gIcon.table.title.c_str(),&bbox);
              
              cairo_rel_move_to(cairo,-0.5*bbox.width,0.5*bbox.height);
              cairo_show_text(cairo,gIcon.table.title.c_str());
              cairo_restore(cairo);
            }
          

          // render the variables
          cairo_identity_matrix(cairo);
          cairo_translate(cairo,0.5*cairoSurface->width(),0.5*cairoSurface->height());
          DrawVars drawVars(cairo, gIcon.x(), gIcon.y(), gIcon.zoomFactor());
          drawVars(gIcon.flowVars); 
          drawVars(gIcon.stockVars); 

          cairoSurface->blit();
        }
    }

    struct DrawVars
    {
      cairo_t* cairo;
      float x, y; // position of this icon
      float left, right; //left & right side of bounding box. Adjusted by this
      float top, bottom; //left & right side of bounding box. Adjusted by this
      float zoomFactor;
      DrawVars(cairo_t* cairo, float x, float y, float zoomFactor): 
        cairo(cairo), x(x), y(y), zoomFactor(zoomFactor) {}
      
      void operator()(const GodleyIcon::Variables& vars)
      {
        for (GodleyIcon::Variables::const_iterator v=vars.begin(); 
             v!=vars.end(); ++v)
          {
            cairo_save(cairo);
            const VariableBase& vv=**v;
            // coordinates of variable within the cairo context
            cairo_translate(cairo, (vv.x()-x), (vv.y()-y));
            cairo_scale(cairo, zoomFactor, zoomFactor);
            RenderVariable rv(*v, cairo, zoomFactor, zoomFactor);
            rv.draw();
            cairo_restore(cairo);
            // adjust bounding box
            float varLeft=vv.x()-x-rv.width()-3; 
            float varBottom=vv.y()-y+rv.width()+3; 
            if (varLeft<left) left=varLeft;
            if (varBottom>bottom) bottom=varBottom;

          }
      }
    };

    // adjust ports so that they align with the image
    void adjustPorts(GodleyIcon::Variables& vars, float dx, float dy)
    {
      for (GodleyIcon::Variables::iterator v=vars.begin(); v!=vars.end(); ++v)
            {
              array<int> ports=(*v)->ports();
              for (int i=0; i<ports.size(); ++i)
                portManager().movePort(ports[i], -2*dx, -2*dy);
            }
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
    int r=createImage<GodleyIconItem>(interp,canvas,itemPtr,objc,objv);
    if (r==TCL_OK)
      {
        GodleyIconItem* xglItem=(GodleyIconItem*)(tkXGLItem->cairoItem);
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
    static Tk_ItemType godleyIconType = xglItemType();
    godleyIconType.name="godley";
    godleyIconType.createProc=creatProc;
    Tk_CreateItemType(&godleyIconType);
    return 0;
  }
#ifdef __GNUC__
#pragma GCC pop
#endif

  void updateVars(GodleyIcon::Variables& vars, 
                  const vector<string>& varNames, 
                  VariableBase::Type varType)
  {
    // update the map of variables from the Godley table
    set<VariablePtr, OrderByName> oldVars(vars.begin(), vars.end());

    vars.clear();
    for (vector<string>::const_iterator nm=varNames.begin(); nm!=varNames.end(); ++nm)
      {
        VariablePtr newVar(varType, *nm);
        newVar->m_godley=true;
        set<VariablePtr>::const_iterator v=oldVars.find(newVar);
        if (v==oldVars.end())
          {
            // add new variable
            vars.push_back(newVar);
            minsky::minsky().variables.addVariable(newVar);
          }
        else
          {
            // copy existing variable
            vars.push_back(*v);
            assert(*v);
          }
      }
    // remove any previously existing variables
    set<string> svName(varNames.begin(),varNames.end()) ;
    for (set<VariablePtr>::iterator v=oldVars.begin(); v!=oldVars.end(); ++v)
      if (svName.count((*v)->name)==0)
        minsky::minsky().variables.erase(*v);
  }

  // determine the width and maximum height on screen of variables in vars
  void accumulateWidthHeight(const GodleyIcon::Variables& vars,
                             float& height, float& width)
  {
    float h=0;
    for (GodleyIcon::Variables::const_iterator v=vars.begin(); v!=vars.end(); ++v)
      {
        RenderVariable rv(*v);
        h+=2*rv.height();
        if (h>height) height=h;
        float w=2*rv.width()+2;
        if (w>width) width=w;
      }
  }
}

inline bool isDotOrDigit(char x)
{return x=='.' || isdigit(x);}

void GodleyIcon::update()
{
  updateVars(stockVars, table.getColumnVariables(), VariableType::stock);
  updateVars(flowVars, table.getVariables(), VariableType::flow);

  // retrieve initial conditions, if any
  for (int r=1; r<table.rows(); ++r)
    if (table.initialConditionRow(r))
      for (int c=1; c<table.cols(); ++c)
        {
          string name=table.cell(0,c);
          stripNonAlnum(name);
          VariableValue& v=minsky().variables.getVariableValue(name);
          v.godleyOverridden=false;
          string::size_type start=table.cell(r,c).find_first_not_of(" ");
          if (start!=string::npos)
            {
              string val=table.cell(r,c).substr(start); //strip spaces
                  
              // only override if a sensible floating number provided
              if (val.size()==1 && isdigit(val[0]) ||
                  val.size()>1 && (isDotOrDigit(val[0]) || 
                                   val[0]=='-' && (isDotOrDigit(val[1]))))
                {
                  istringstream is(val);
                  is >> v.init;
                  if (table.signConventionReversed(c))
                    v.init=-v.init;
                  v.godleyOverridden=true;
                }
            }
          else
            {
              // populate cell with current variable's initial value
              ostringstream val;
              val << (table.signConventionReversed(c)? -v.init: v.init);
              table.cell(r,c)=val.str();
              v.godleyOverridden=true;
            }
        }

  // determine height of variables part of icon
  float height=0;
  stockMargin=0;
  flowMargin=0;
  accumulateWidthHeight(stockVars, height, stockMargin);
  accumulateWidthHeight(flowVars, height, flowMargin);
  iconSize=max(100.0, 1.8*height);

  // position of margin in absolute canvas coordinate
  float x= this->x() - 0.5*(0.8*iconSize-flowMargin)*zoomFactor();
  float y= this->y() - 0.37*iconSize*zoomFactor();
  for (Variables::iterator v=flowVars.begin(); v!=flowVars.end(); ++v)
    {
      // right justification
      RenderVariable rv(*v);
      const_cast<VariablePtr&>(*v)->MoveTo(x-rv.width()*zoomFactor(),y);
      y+=2*RenderVariable(*v).height()*zoomFactor();
    }
  x=this->x() - 0.5*(0.85*iconSize-flowMargin)*zoomFactor();
  y=this->y() + 0.5*(iconSize-stockMargin)*zoomFactor();

  for (Variables::iterator v=stockVars.begin(); v!=stockVars.end(); ++v)
    {
      // top justification at bottom of icon
      RenderVariable rv(*v);
      //OK because we're not changing variable name
      VariableBase& vv=const_cast<VariableBase&>(**v); 
      vv.MoveTo(x,y+rv.width()*zoomFactor());
      vv.rotation=90;
      x+=2*rv.height()*zoomFactor();
    }
}

int GodleyIcon::numPorts() const
{
  int numPorts=0;
  for (Variables::const_iterator v=flowVars.begin(); v!=flowVars.end(); ++v)
    numPorts+=(*v)->numPorts();
  for (Variables::const_iterator v=stockVars.begin(); v!=stockVars.end(); ++v)
    numPorts+=(*v)->numPorts();
  return numPorts;
}

array<int> GodleyIcon::ports() const
{
  array<int> ports;
  for (Variables::const_iterator v=flowVars.begin(); v!=flowVars.end(); ++v)
    ports<<=(*v)->ports();
  for (Variables::const_iterator v=stockVars.begin(); v!=stockVars.end(); ++v)
    ports<<=(*v)->ports();
  return ports;
}

void GodleyIcon::MoveTo(float x1, float y1)
{
  float dx=x1-x(), dy=y1-y();
  m_x=x1; m_y=y1;
  //const_cast OK below because location doesn't affect ordering
   for (Variables::iterator v=flowVars.begin(); v!=flowVars.end(); ++v)
     const_cast<VariableBase&>(**v).move(dx, dy); 
   for (Variables::iterator v=stockVars.begin(); v!=stockVars.end(); ++v)
     const_cast<VariableBase&>(**v).move(dx, dy);
}


int GodleyIcon::Select(float x, float y)
{
  for (Variables::iterator v=flowVars.begin(); v!=flowVars.end(); ++v)
    if (RenderVariable(*v).inImage(x,y)) 
      return minsky().variables.getIDFromVariable(*v);
  for (Variables::iterator v=stockVars.begin(); v!=stockVars.end(); ++v)
    if (RenderVariable(*v).inImage(x,y)) 
      return minsky().variables.getIDFromVariable(*v);
  return -1;
}

void GodleyIcon::zoom(float xOrigin, float yOrigin,float factor) {
  minsky::zoom(m_x, xOrigin, factor);
  minsky::zoom(m_y, yOrigin, factor);
  m_zoomFactor*=factor;
  update();
  array<int> pp=ports();
}


static int dum=(initVec().push_back(registerItem), 0);
