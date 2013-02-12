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
// Implementations of canvas items representing operations and variables.

#include "operation.h"
#include "minsky.h"
#include "init.h"
#include "cairoItems.h"
#include <cairo_base.h>
#include <arrays.h>
#include <ecolab_epilogue.h>
#include <boost/geometry/geometry.hpp>

using namespace ecolab::cairo;
using namespace ecolab;
using namespace std;
using namespace minsky;
using namespace boost::geometry;

namespace 
{
  // we need some extra fields to handle the additional options
  struct TkMinskyItem: public ImageItem
  {
    int id; // identifier of the C++ object this item represents
  };

  static Tk_CustomOption tagsOption = {
    (Tk_OptionParseProc *) Tk_CanvasTagsParseProc,
    Tk_CanvasTagsPrintProc, (ClientData) NULL
  };

  struct MinskyItemImage: public CairoImage
  {
    MinskyItemImage(const string& image): CairoImage(image) {}
    static Tk_ConfigSpec configSpecs[];
  };

  struct OperationItem: public MinskyItemImage
  {
    OperationItem(const std::string& image): MinskyItemImage(image) {}
    OperationPtr op;    
    cairo_t * cairo;


    void draw()
    {
      if (cairoSurface && op)
        {
          cairoSurface->clear();
          cairo=cairoSurface->cairo();
          string label;
          cairo_reset_clip(cairo);
          xScale=yScale=op->zoomFactor;
          initMatrix();
          cairo_select_font_face(cairo, "sans-serif", CAIRO_FONT_SLANT_ITALIC,
                                 CAIRO_FONT_WEIGHT_NORMAL);
          cairo_set_font_size(cairo,12);
          cairo_set_line_width(cairo,1);


          RenderOperation(op,cairo,xScale,yScale).draw();

          redrawIfSurfaceTooSmall();

          {
            array<double> bbox=boundingBox();

            // not sure why we need to do this
            double w=bbox[2]-bbox[0], h=bbox[3]-bbox[1];
            if (bbox[0]<0) bbox[0]=0;
            if (bbox[1]<0) bbox[1]=0;

            assert(w<=cairoSurface->width());
            assert(h<=cairoSurface->height());

            cairoSurface->blit
              (bbox[0], bbox[1], w, h);
          }
        }
    }
  };

  struct VariableItem: public MinskyItemImage
  {
    VariableItem(const std::string& image): MinskyItemImage(image) {}
    VariablePtr var;
    void draw()
    {
      if (cairoSurface && var)
        {
          cairoSurface->clear();
          cairo_t *cairo=cairoSurface->cairo();
          xScale=yScale=var->zoomFactor;

          cairo_reset_clip(cairo);
          initMatrix();
          cairo_set_line_width(cairo,1);

          RenderVariable rv(var, cairo, xScale, yScale);
          rv.draw();
          redrawIfSurfaceTooSmall();
          {
            array<double> bbox=boundingBox();

            // not sure why we need to do this
            double w=bbox[2]-bbox[0], h=bbox[3]-bbox[1];
            if (bbox[0]<0) bbox[0]=0;
            if (bbox[1]<0) bbox[1]=0;

            assert(w<=cairoSurface->width());
            assert(h<=cairoSurface->height());

            cairoSurface->blit
              (bbox[0], bbox[1], w, h);
          }
       }
    }
  };

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
  // causes problems on MacOSX
  //#undef Tk_Offset
  //#define Tk_Offset(type, field) ((int) ((char *) &((type *) 1)->field)-1)

  Tk_ConfigSpec MinskyItemImage::configSpecs[] =
    {
      {TK_CONFIG_STRING, "-image", NULL, NULL,
       NULL, Tk_Offset(ImageItem, imageString), 0},
      {TK_CONFIG_DOUBLE, "-scale", NULL, NULL,
       "1.0", Tk_Offset(ImageItem, scale), TK_CONFIG_NULL_OK},
      {TK_CONFIG_DOUBLE, "-rotation", NULL, NULL,
       "0.0", Tk_Offset(ImageItem, rotation), TK_CONFIG_NULL_OK},
      {TK_CONFIG_INT, "-id", NULL, NULL,
       NULL, Tk_Offset(TkMinskyItem, id), 0},
      {TK_CONFIG_CUSTOM, "-tags", NULL, NULL,
       NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
      {TK_CONFIG_END}
    };

  int operationCreatProc(Tcl_Interp *interp, Tk_Canvas canvas, 
                         Tk_Item *itemPtr, int objc,Tcl_Obj *CONST objv[])
  {
    TkMinskyItem* tkMinskyItem=(TkMinskyItem*)(itemPtr);
    tkMinskyItem->id=-1;
    int r=createImage<OperationItem>(interp,canvas,itemPtr,objc,objv);
    if (r==TCL_OK && tkMinskyItem->id>=0)
      {
        OperationItem* opItem=(OperationItem*)(tkMinskyItem->cairoItem);
        if (opItem) 
          {
            opItem->op=minsky::minsky().operations[tkMinskyItem->id];
            opItem->draw();
            TkImageCode::ComputeImageBbox(canvas, tkMinskyItem);
          }
      }
    return r;
  }

  int varCreatProc(Tcl_Interp *interp, Tk_Canvas canvas, 
                         Tk_Item *itemPtr, int objc,Tcl_Obj *CONST objv[])
  {
    TkMinskyItem* tkMinskyItem=(TkMinskyItem*)(itemPtr);
    tkMinskyItem->id=-1;
    int r=createImage<VariableItem>(interp,canvas,itemPtr,objc,objv);
    if (r==TCL_OK && tkMinskyItem->id>=0)
      {
        VariableItem* varItem=dynamic_cast<VariableItem*>(tkMinskyItem->cairoItem);
        if (varItem) 
          {
            varItem->var=minsky::minsky().variables[tkMinskyItem->id];
            varItem->draw();
            TkImageCode::ComputeImageBbox(canvas, tkMinskyItem);
          }
      }
    return r;
  }

  // overrride cairoItem's configureProc to process the extra config options
  int configureProc(Tcl_Interp *interp,Tk_Canvas canvas,Tk_Item *itemPtr,
                    int objc,Tcl_Obj *CONST objv[],int flags)
  {
    return TkImageCode::configureCairoItem
      (interp,canvas,itemPtr,objc,objv,flags, MinskyItemImage::configSpecs);
  }

  // register OperatorItem with Tk for use in canvases.
  int registerItems()
  {
    static Tk_ItemType operationItemType = cairoItemType;
    operationItemType.name="operation";
    operationItemType.itemSize=sizeof(TkMinskyItem);
    operationItemType.createProc=operationCreatProc;
    operationItemType.configProc=configureProc;
    operationItemType.configSpecs=MinskyItemImage::configSpecs;
    Tk_CreateItemType(&operationItemType);

    static Tk_ItemType varItemType = operationItemType;
    varItemType.name="variable";
    varItemType.createProc=varCreatProc;
    Tk_CreateItemType(&varItemType);
    return 0;
  }
}


static int dum=(initVec().push_back(registerItems), 0);

RenderOperation::RenderOperation(const OperationPtr& op, cairo_t* cairo,
                               double xScale, double yScale):
  op(op), cairo(cairo), xScale(xScale), yScale(yScale)
{
  cairo_t *lcairo=cairo;
  cairo_surface_t* surf=NULL;
  if (!lcairo)
    {
      surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100,100);
      lcairo = cairo_create(surf);
    }

  assert(op);
  const float l=op->l, r=op->r;
  w=0.5*(-l+r);
  h=op->h;

  switch (op->type())
    {
    case OperationType::constant:
      {
        cairo_text_extents_t bbox;
        Constant *c=dynamic_cast<Constant*>(op.get());
        assert(c);
        cairo_text_extents(lcairo,c->description.c_str(),&bbox);
        w=0.5*bbox.width+2; 
        h=0.5*bbox.height+2;
        break;
      }
    case OperationType::integrate:
      {
        IntOp* i=dynamic_cast<IntOp*>(op.get());
        assert(i);
        if (i->coupled())
          {
            RenderVariable rv(i->getIntVar(),cairo);
            w+=i->intVarOffset+rv.width(); 
            h=max(h, rv.height());
          }
        break;
      }
    }
 if (surf) //cleanup temporary surface
    {
      cairo_destroy(lcairo);
      cairo_surface_destroy(surf);
    }
}

Polygon RenderOperation::geom() const
{
  Rotate rotate(op->rotation, op->x(), op->y());
  float zl=op->l*op->zoomFactor, zh=op->h*op->zoomFactor, 
    zr=op->r*op->zoomFactor;
  Polygon r;
  // TODO: handle bound integration variables, and constants
  r+= rotate(op->x()+zl, op->y()-zh), rotate(op->x()+zl, op->y()+zh), 
    rotate(op->x()+zr, op->y());
  correct(r);
  return r;
}

namespace
{
  struct DrawBinOp
  {
    cairo_t *cairo;
    DrawBinOp(cairo_t *cairo): cairo(cairo) {}

    void drawPlus() const
    {
      cairo_move_to(cairo,0,-5);
      cairo_line_to(cairo,0,5);
      cairo_move_to(cairo,-5,0);
      cairo_line_to(cairo,5,0);
      cairo_stroke(cairo);
    }

    void drawMinus() const
    {
      cairo_move_to(cairo,-5,0);
      cairo_line_to(cairo,5,0);
      cairo_stroke(cairo);
    }

    void drawMultiply() const
    {
      cairo_move_to(cairo,-5,-5);
      cairo_line_to(cairo,5,5);
      cairo_move_to(cairo,-5,5);
      cairo_line_to(cairo,5,-5);
      cairo_stroke(cairo);
    }

    void drawDivide() const
    {
      cairo_move_to(cairo,-5,0);
      cairo_line_to(cairo,5,0);
      cairo_new_sub_path(cairo);
      cairo_arc(cairo,0,3,1,0,2*M_PI);
      cairo_new_sub_path(cairo);
      cairo_arc(cairo,0,-3,1,0,2*M_PI);
      cairo_stroke(cairo);
    }

    // puts a small symbol to identify port
    // x, y = position of symbol
    void drawPort(void (DrawBinOp::*symbol)() const, float x, float y, float rotation)  const
    {
      cairo_save(cairo);
      
      double angle=rotation * M_PI / 180.0;
      double fm=std::fmod(rotation,360);
      if (!(fm>-90 && fm<90 || fm>270 || fm<-270))
        y=-y;
      cairo_rotate(cairo, angle);
      
      cairo_translate(cairo,0.7*x,0.6*y);
      cairo_scale(cairo,0.5,0.5);
      
      // and counter-rotate
      cairo_rotate(cairo, -angle);
      (this->*symbol)();
      cairo_restore(cairo);
    }
  };
}

namespace minsky
{

  // virtual draw methods for operations - defined here rather than
  // operations.cc because it is more related to the functionality in
  // this file.

  template <> void Operation<OperationType::constant>::draw(cairo_t* cairo) const
  {
    assert(false); //shouldn't be here
  }

  template <> void Operation<OperationType::time>::draw(cairo_t* cairo) const
  {
    cairo_move_to(cairo,-4,2);
    cairo_show_text(cairo,"t");
  }

  template <> void Operation<OperationType::copy>::draw(cairo_t* cairo) const
  {
    cairo_move_to(cairo,-4,2);
    cairo_show_text(cairo,"=");
  }

  template <> void Operation<OperationType::integrate>::draw(cairo_t* cairo) const
  {
    cairo_move_to(cairo,-7,4.5);
    cairo_show_text(cairo,"\xE2\x88\xAB");
    cairo_show_text(cairo,"dt");
  }

  template <> void Operation<OperationType::sqrt>::draw(cairo_t* cairo) const
  {
    cairo_save(cairo);
    cairo_set_font_size(cairo,10);   
    cairo_move_to(cairo,-7,6);
    cairo_show_text(cairo,"\xE2\x88\x9a");
    cairo_set_line_width(cairo,0.5);
    cairo_rel_move_to(cairo,0,-9);
    cairo_rel_line_to(cairo,5,0);
    cairo_set_source_rgb(cairo,0,0,0);
    cairo_stroke(cairo);
    cairo_restore(cairo);
    //    cairo_show_text(cairo,"sqrt");
  }

  template <> void Operation<OperationType::exp>::draw(cairo_t* cairo) const
  {
    cairo_move_to(cairo,-7,3);
    cairo_show_text(cairo,"e");
    cairo_rel_move_to(cairo,0,-4);
    cairo_set_font_size(cairo,7);
    cairo_show_text(cairo,"x");
  }

  template <> void Operation<OperationType::pow>::draw(cairo_t* cairo) const
  {
    cairo_move_to(cairo,-6,3);
    cairo_show_text(cairo,"x");
    cairo_rel_move_to(cairo,0,-4);
    cairo_set_font_size(cairo,7);
    cairo_show_text(cairo,"y");
    cairo_set_font_size(cairo,5);
    cairo_move_to(cairo, l+1, -h+6);
    cairo_show_text(cairo,"x");
    cairo_move_to(cairo, l+1, h-3);
    cairo_show_text(cairo,"y");
  }

  template <> void Operation<OperationType::ln>::draw(cairo_t* cairo) const
  {
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo," ln");
  }

  template <> void Operation<OperationType::log>::draw(cairo_t* cairo) const
  {
    cairo_set_font_size(cairo,10);
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo,"log");
    cairo_rel_move_to(cairo,0,3);
    cairo_set_font_size(cairo,7);
    cairo_show_text(cairo,"b");
    cairo_set_font_size(cairo,5);
    cairo_move_to(cairo, l+1, -h+6);
    cairo_show_text(cairo,"x");
    cairo_move_to(cairo, l+1, h-3);
    cairo_show_text(cairo,"b");
  
  }

  template <> void Operation<OperationType::sin>::draw(cairo_t* cairo) const
  {
    cairo_set_font_size(cairo,10);
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo,"sin");
  }

  template <> void Operation<OperationType::cos>::draw(cairo_t* cairo) const
  {
    cairo_set_font_size(cairo,10);
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo,"cos");
  }

  template <> void Operation<OperationType::tan>::draw(cairo_t* cairo) const
  {
    cairo_set_font_size(cairo,10);
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo,"tan");
  }

  template <> void Operation<OperationType::asin>::draw(cairo_t* cairo) const
  {
    cairo_set_font_size(cairo,9);
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo,"sin");
    cairo_rel_move_to(cairo,0,-3);
    cairo_set_font_size(cairo,7);
    cairo_show_text(cairo,"-1");
    cairo_rel_move_to(cairo,0,-2);
  }

  template <> void Operation<OperationType::acos>::draw(cairo_t* cairo) const
  {
    cairo_set_font_size(cairo,9);
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo,"cos");
    cairo_rel_move_to(cairo,0,-3);
    cairo_set_font_size(cairo,7);
    cairo_show_text(cairo,"-1");
    cairo_rel_move_to(cairo,0,-2);
  }

  template <> void Operation<OperationType::atan>::draw(cairo_t* cairo) const
  {
    cairo_set_font_size(cairo,9);
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo,"tan");
    cairo_rel_move_to(cairo,0,-3);
    cairo_set_font_size(cairo,7);
    cairo_show_text(cairo,"-1");
    cairo_rel_move_to(cairo,0,-2);
  }

  template <> void Operation<OperationType::sinh>::draw(cairo_t* cairo) const
  {
    cairo_set_font_size(cairo,8);
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo,"sinh");
  }

  template <> void Operation<OperationType::cosh>::draw(cairo_t* cairo) const
  {
    cairo_set_font_size(cairo,8);
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo,"cosh");
  }

  template <> void Operation<OperationType::tanh>::draw(cairo_t* cairo) const
  {
    cairo_set_font_size(cairo,8);
    cairo_move_to(cairo,-9,3);
    cairo_show_text(cairo,"tanh");
  }

  template <> void Operation<OperationType::add>::draw(cairo_t* cairo) const
  {
    DrawBinOp d(cairo);
    d.drawPlus();
    d.drawPort(&DrawBinOp::drawPlus, l, h, rotation);
    d.drawPort(&DrawBinOp::drawPlus, l, -h, rotation);
  }

  template <> void Operation<OperationType::subtract>::draw(cairo_t* cairo) const
  {
    DrawBinOp d(cairo);
    d.drawMinus();
    d.drawPort(&DrawBinOp::drawPlus, l, -h, rotation);
    d.drawPort(&DrawBinOp::drawMinus, l, h, rotation);
  }

  template <> void Operation<OperationType::multiply>::draw(cairo_t* cairo) const
  {
    DrawBinOp d(cairo);
    d.drawMultiply();
    d.drawPort(&DrawBinOp::drawMultiply, l, h, rotation);
    d.drawPort(&DrawBinOp::drawMultiply, l, -h, rotation);
  }

  template <> void Operation<OperationType::divide>::draw(cairo_t* cairo) const
  {
    DrawBinOp d(cairo);
    d.drawDivide();
    d.drawPort(&DrawBinOp::drawMultiply, l, -h, rotation);
    d.drawPort(&DrawBinOp::drawDivide, l, h, rotation);
  }

  template <> void Operation<OperationType::numOps>::draw(cairo_t* cairo) const
  {/* needs to be here, and is actually called */}

}

void RenderOperation::draw()
{
  const float l=op->l, r=op->r;
  // if rotation is in 1st or 3rd quadrant, rotate as
  // normal, otherwise flip the text so it reads L->R
  double angle=op->rotation * M_PI / 180.0;
  double fm=std::fmod(op->rotation,360);
  bool textFlipped=!(fm>-90 && fm<90 || fm>270 || fm<-270);
  
  switch (op->type())
    {
      // at the moment its too tricky to get all the information
      // together for rendering constants
    case OperationType::constant:
      {
        Constant *c=dynamic_cast<Constant*>(op.get());
        assert(c);
        cairo_save(cairo);
        cairo_rotate(cairo, angle + (textFlipped? M_PI: 0));
        
        cairo_move_to(cairo,-w+1,h-4);
        cairo_show_text(cairo,c->description.c_str());
        cairo_restore(cairo);
        cairo_rotate(cairo, angle);
               
        cairo_set_source_rgb(cairo,0,0,1);
        cairo_move_to(cairo,-w,-h);
        cairo_line_to(cairo,-w,h);
        cairo_line_to(cairo,w,h);

        cairo_line_to(cairo,w+2,0);
        cairo_line_to(cairo,w,-h);
        cairo_close_path(cairo);
        cairo_clip_preserve(cairo);
        cairo_stroke(cairo);

        // set the output ports coordinates
        // compute port coordinates relative to the icon's
        // point of reference
        double x=w+2, y=0;
        cairo_save(cairo);
        cairo_identity_matrix(cairo);
        cairo_scale(cairo, xScale, yScale);
        cairo_rotate(cairo, angle);
        cairo_user_to_device(cairo, &x, &y);
        cairo_restore(cairo);

        portManager().movePortTo
          (op->ports()[0], op->x()+x, op->y()+y);
        return;
    }
    case OperationType::integrate:
      if (IntOp* i=dynamic_cast<IntOp*>(op.get()))
        if (i->coupled())
          {
            // we need to add some translation if the variable is bound
            RenderVariable rv(i->getIntVar(),cairo);
            cairo_rotate(cairo,op->rotation*M_PI/180.0);
            cairo_translate(cairo, -0.5*(i->intVarOffset+2*rv.width()+2+r),0);
            cairo_rotate(cairo,-op->rotation*M_PI/180.0);
          }
      op->draw(cairo);
      break;
    default:
      op->draw(cairo);
      break;
    }

  int intVarWidth=0, intVarHeight=0;
  cairo_rotate(cairo, angle);
  if (IntOp* i=dynamic_cast<IntOp*>(op.get()))
    if (i->coupled())
      {
        cairo_move_to(cairo,r,0);
        cairo_line_to(cairo,r+10,0);
        cairo_stroke(cairo);
        // display an integration variable next to it
        RenderVariable rv(i->getIntVar(),cairo);
        // save the render width for later use in setting the clip
        intVarWidth=rv.width(); intVarHeight=2*rv.height();
        // set the port location...
        i->getIntVar()->MoveTo(i->x()+i->intVarOffset, i->y());
            
        cairo_save(cairo);
        cairo_translate(cairo,r+i->intVarOffset+intVarWidth,0);
        // to get text to render correctly, we need to set
        // the var's rotation, then antirotate it
        i->getIntVar()->rotation=i->rotation;
        cairo_rotate(cairo, -M_PI*i->rotation/180.0);
        rv.draw();
        cairo_restore(cairo);
      }
  cairo_move_to(cairo,l,h);
  cairo_line_to(cairo,l,-h);
  cairo_line_to(cairo,r,0);
              
  cairo_close_path(cairo);

  cairo_save(cairo);
  cairo_set_source_rgb(cairo,0,0,1);
  cairo_stroke_preserve(cairo);
  cairo_restore(cairo);
  if (IntOp* i=dynamic_cast<IntOp*>(op.get()))
    if (i->coupled())
      {
        // we need to add some additional clip region to
        // cover the variable
        cairo_new_path(cairo);
        cairo_rectangle
          (cairo,l,-intVarHeight, i->intVarOffset+2*intVarWidth+2+r-l, 
           2*intVarHeight);
      }
  cairo_clip(cairo);

  // compute port coordinates relative to the icon's
  // point of reference
  double x0=r, y0=0, x1=l, y1=op->numPorts() > 2? -h+3: 0, 
    x2=l, y2=h-3;
                  
  if (textFlipped) swap(y1,y2);

  // adjust for integration variable
  if (IntOp* i=dynamic_cast<IntOp*>(op.get()))
    if (i->coupled())
      x0+=i->intVarOffset+2*intVarWidth;
  // adjust so that 0 is in centre
  float dx=0.5*(x0+l);
  x0-=dx;
  x1-=dx;
  x2-=dx;

  cairo_save(cairo);
  cairo_identity_matrix(cairo);
  cairo_translate(cairo, op->x(), op->y());
  cairo_scale(cairo,xScale,yScale);
  cairo_rotate(cairo, angle);
  cairo_user_to_device(cairo, &x0, &y0);
  cairo_user_to_device(cairo, &x1, &y1);
  cairo_user_to_device(cairo, &x2, &y2);
  cairo_restore(cairo);

  if (op->numPorts()>0) 
    portManager().movePortTo(op->ports()[0], x0, y0);
  if (op->numPorts()>1) 
    portManager().movePortTo(op->ports()[1], x1, y1);
  if (op->numPorts()>2)
    portManager().movePortTo(op->ports()[2], x2, y2);
 
}


RenderVariable::RenderVariable(const VariablePtr& var, cairo_t* cairo,
                               double xScale, double yScale):
  var(var), cairo(cairo), xScale(xScale), yScale(yScale)
{
  cairo_t *lcairo=cairo;
  cairo_surface_t* surf=NULL;
  if (!lcairo)
    {
      surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 100,100);
      lcairo = cairo_create(surf);
    }

  cairo_select_font_face(lcairo, "sans-serif", CAIRO_FONT_SLANT_ITALIC,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(lcairo,12);
  cairo_text_extents_t bbox;
  cairo_text_extents(lcairo,var->Name().c_str(),&bbox);
  w=0.5*bbox.width+2; 
  h=0.5*bbox.height+2;

  if (surf) //cleanup temporary surface
    {
      cairo_destroy(lcairo);
      cairo_surface_destroy(surf);
    }
}

Polygon RenderVariable::geom() const
{
  float x=var->x(), y=var->y();
  float wz=w*var->zoomFactor, hz=h*var->zoomFactor;
  Rotate rotate(var->rotation, x, y);

  Polygon r;
  r+= rotate(x-wz, y-hz), rotate(x-wz, y+hz), 
    rotate(x+wz, y+hz), rotate(x+wz, y-hz);
  correct(r);
  return r;
}

void RenderVariable::draw()
{
  if (cairo)
    {
      double angle=var->rotation * M_PI / 180.0;
      double fm=std::fmod(var->rotation,360);

      cairo_save(cairo);
      // if rotation is in 1st or 3rd quadrant, rotate as
      // normal, otherwise flip the text so it reads L->R
      if (fm>-90 && fm<90 || fm>270 || fm<-270)
        cairo_rotate(cairo, angle);
      else
        cairo_rotate(cairo, angle+M_PI);

      cairo_move_to(cairo,-w+1,h-2);
      cairo_show_text(cairo,var->Name().c_str());
      cairo_restore(cairo);
      cairo_rotate(cairo, angle);
      cairo_set_source_rgb(cairo,1,0,0);
      cairo_move_to(cairo,-w,-h);
      if (var->lhs())
        cairo_line_to(cairo,-w+2,0);
      cairo_line_to(cairo,-w,h);
      cairo_line_to(cairo,w,h);
      cairo_line_to(cairo,w+2,0);
      cairo_line_to(cairo,w,-h);
      cairo_close_path(cairo);
      cairo_clip_preserve(cairo);
      cairo_stroke(cairo);
  
      updatePortLocs();
    }
}

void RenderVariable::updatePortLocs()
{
  double angle=var->rotation * M_PI / 180.0;
  double x0=w, y0=0, x1=-w+2, y1=0;
  double sa=sin(angle), ca=cos(angle);
  portManager().movePortTo(var->outPort(), 
                           var->x()+xScale*(x0*ca-y0*sa), 
                           var->y()+xScale*(y0*ca+x0*sa));
  portManager().movePortTo(var->inPort(), 
                           var->x()+xScale*(x1*ca-y1*sa), 
                           var->y()+xScale*(y1*ca+x1*sa));
}


bool RenderVariable::inImage(float x, float y)
{
  float dx=x-var->x(), dy=y-var->y();
  float rx=dx*cos(var->rotation*M_PI/180)-dy*sin(var->rotation*M_PI/180);
  float ry=dy*cos(var->rotation*M_PI/180)+dx*sin(var->rotation*M_PI/180);
  return rx>=-w && rx<=w && ry>=-h && ry <= h;
}


void minsky::drawTriangle
(cairo_t* cairo, double x, double y, cairo::Colour& col, double angle)
{
  cairo_save(cairo);
  cairo_new_path(cairo);
  cairo_set_source_rgba(cairo,col.r,col.g,col.b,col.a);
  cairo_translate(cairo,x,y);
  cairo_rotate(cairo, angle);
  cairo_move_to(cairo,10,0);
  cairo_line_to(cairo,0,-3);
  cairo_line_to(cairo,0,3);
  cairo_fill(cairo);
  cairo_restore(cairo);
}

