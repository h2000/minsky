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
using namespace ecolab::cairo;
using namespace ecolab;
//using namespace ecolab::array_ns;
using namespace std;
using namespace minsky;

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
    OperationItem(const std::string& image): MinskyItemImage(image), op(NULL) {}
    Operation* op;    
    cairo_t * cairo;

    void drawPlus()
    {
      cairo_move_to(cairo,0,-5);
      cairo_line_to(cairo,0,5);
      cairo_move_to(cairo,-5,0);
      cairo_line_to(cairo,5,0);
      cairo_stroke(cairo);
    }

    void drawMinus()
    {
      cairo_move_to(cairo,-5,0);
      cairo_line_to(cairo,5,0);
      cairo_stroke(cairo);
    }

    void drawMultiply()
    {
      cairo_move_to(cairo,-5,-5);
      cairo_line_to(cairo,5,5);
      cairo_move_to(cairo,-5,5);
      cairo_line_to(cairo,5,-5);
      cairo_stroke(cairo);
    }

    void drawDivide()
    {
      cairo_move_to(cairo,-5,0);
      cairo_line_to(cairo,5,0);
      cairo_move_to(cairo,0,3);
      cairo_arc(cairo,0,3,1,0,2*M_PI);
      cairo_move_to(cairo,0,-3);
      cairo_arc(cairo,0,-3,1,0,2*M_PI);
      cairo_stroke(cairo);
    }

    // puts a small symbol to identify port
    // x, y = position of symbol
    void drawPort(void (OperationItem::*symbol)(), float x, float y) 
    {
      cairo_save(cairo);
        
      double angle=op->rotation * M_PI / 180.0;
      double fm=std::fmod(op->rotation,360);
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

    void draw()
    {
      if (cairoSurface && op)
        {
          cairoSurface->clear();
          cairo=cairoSurface->cairo();
          string label;
          const float l=op->l, h=op->h, r=op->r;

          cairo_reset_clip(cairo);
          cairo_save(cairo);
          cairo_identity_matrix(cairo);
          cairo_select_font_face(cairo, "sans-serif", CAIRO_FONT_SLANT_ITALIC,
                                 CAIRO_FONT_WEIGHT_NORMAL);
          cairo_set_font_size(cairo,12);
          cairo_translate(cairo,.5*cairoSurface->width(),
                          0.5*cairoSurface->height());
          cairo_set_line_width(cairo,1);

          // if rotation is in 1st or 3rd quadrant, rotate as
          // normal, otherwise flip the text so it reads L->R
          double angle=op->rotation * M_PI / 180.0;
          double fm=std::fmod(op->rotation,360);
          bool textFlipped=!(fm>-90 && fm<90 || fm>270 || fm<-270);

          switch (op->type())
            {
            case Operation::constant:
              {
                cairo_text_extents_t bbox;
                cairo_text_extents(cairo,op->description().c_str(),&bbox);
                float w=0.5*bbox.width+2; 
                float h=0.5*bbox.height+2;
                cairo_save(cairo);
                cairo_rotate(cairo, angle + (textFlipped? M_PI: 0));

                cairo_move_to(cairo,-w+1,h-4);
                cairo_show_text(cairo,op->description().c_str());
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
                //                cairo_restore(cairo);

                // set the output ports coordinates
                // compute port coordinates relative to the icon's
                // point of reference
                double x=w+2, y=0;
                cairo_save(cairo);
                cairo_identity_matrix(cairo);
                cairo_rotate(cairo, angle);
                cairo_user_to_device(cairo, &x, &y);
                cairo_restore(cairo);

                portManager().movePortTo
                  (op->ports()[0], op->x+x, op->y+y);
                break;
              }
            case Operation::time:
              cairo_move_to(cairo,-4,2);
              cairo_show_text(cairo,"t");
              break;
            case Operation::copy:
              cairo_move_to(cairo,-4,2);
              cairo_show_text(cairo,"=");
              break;
            case Operation::integrate:
              cairo_move_to(cairo,-7,4.5);
              cairo_show_text(cairo,"\xE2\x88\xAB");
              cairo_show_text(cairo,"dx");
             break;
            case Operation::exp:
              cairo_move_to(cairo,-9,3);
              cairo_show_text(cairo,"e");
              cairo_rel_move_to(cairo,0,-2);
              cairo_show_text(cairo,"x");
              break;
            case Operation::add:
              drawPlus();
              drawPort(&OperationItem::drawPlus, l, h);
              drawPort(&OperationItem::drawPlus, l, -h);
              break;
            case Operation::subtract:
              //label="\xA2\x80\x92"; //doesn't display
              drawMinus();
              drawPort(&OperationItem::drawPlus, l, -h);
              drawPort(&OperationItem::drawMinus, l, h);
              break;
            case Operation::multiply:
              //label="\xE0\x83\x97";
              drawMultiply();
              drawPort(&OperationItem::drawMultiply, l, h);
              drawPort(&OperationItem::drawMultiply, l, -h);
              break;
            case Operation::divide:
              //              label="\xE0\x83\xB7";
              drawDivide();
              drawPort(&OperationItem::drawMultiply, l, -h);
              drawPort(&OperationItem::drawDivide, l, h);
              break;
            }
          int intVarWidth=0, intVarHeight=0;
          if (op->type()!=Operation::constant)
            {
              double angle=op->rotation * M_PI / 180.0;
              cairo_rotate(cairo, angle);
              if (op->coupled())
                {
                  cairo_move_to(cairo,r,0);
                  cairo_line_to(cairo,r+10,0);
                  cairo_stroke(cairo);
                  // display an integration variable next to it
                  RenderVariable rv(op->getIntVar(),cairo);
                  // save the render width for later use in setting the clip
                  intVarWidth=rv.width(); intVarHeight=2*rv.height();
                  // set the port location...
                  op->getIntVar()->MoveTo(op->x+op->intVarOffset, op->y);

                  cairo_save(cairo);
                  cairo_translate(cairo,r+op->intVarOffset+intVarWidth,0);
                  // to get text to render correctly, we need to set
                  // the var's rotation, then antirotate it
                  op->getIntVar()->rotation=op->rotation;
                  cairo_rotate(cairo, -M_PI*op->rotation/180.0);
                  rv.draw();
                  cairo_restore(cairo);
               }
              cairo_move_to(cairo,l,h);
              cairo_line_to(cairo,l,-h);
              cairo_line_to(cairo,r,0);
              
              cairo_close_path(cairo);
              //              cairo_clip_preserve(cairo);

              cairo_save(cairo);
              cairo_set_source_rgb(cairo,0,0,1);
              cairo_stroke_preserve(cairo);
              if (op->coupled())
                {
                  // we need to add some additional clip region to
                  // cover the variable
                  cairo_rectangle
                    (cairo,r,-intVarHeight, op->intVarOffset+2*intVarWidth+2, 
                     2*intVarHeight);
                }
              cairo_restore(cairo);
              cairo_clip(cairo);

              // compute port coordinates relative to the icon's
              // point of reference
              double x0=r, y0=0, x1=l, y1=op->numPorts() > 2? -h+3: 0, 
                x2=l, y2=h-3;
                  
              if (textFlipped) swap(y1,y2);

              // adjust for integration variable
              if (op->coupled())
                x0+=op->intVarOffset+2*intVarWidth;
              // adjust so that 0 is in centre
              float dx=0.5*(x0+l);
              x0-=dx;
              x1-=dx;
              x2-=dx;

              cairo_save(cairo);
              cairo_identity_matrix(cairo);
              cairo_translate(cairo, op->x, op->y);
              cairo_rotate(cairo, angle);
              cairo_user_to_device(cairo, &x0, &y0);
              cairo_user_to_device(cairo, &x1, &y1);
              cairo_user_to_device(cairo, &x2, &y2);
              cairo_restore(cairo);

              portManager().movePortTo(op->ports()[0], x0, y0);
              if (op->numPorts()>1) 
                portManager().movePortTo(op->ports()[1], x1, y1);
              if (op->numPorts()>2)
                portManager().movePortTo(op->ports()[2], x2, y2);
            }

          redrawIfSurfaceTooSmall();
          array<double> bbox=boundingBox();

          cairoSurface->blit
            (bbox[0]+0.5*cairoSurface->width(), 
             bbox[1]+0.5*cairoSurface->height(), 
             bbox[2]-bbox[0], bbox[3]-bbox[1]);
          cairo_restore(cairo);
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

          cairo_reset_clip(cairo);
          cairo_identity_matrix(cairo);
          cairo_translate(cairo,.5*cairoSurface->width(), 
                          0.5*cairoSurface->height());
          cairo_set_line_width(cairo,1);

          RenderVariable rv(var, cairo);
          rv.draw();
          redrawIfSurfaceTooSmall();

          array<double> clipBox=boundingBox();

          double x=clipBox[0]+0.5*cairoSurface->width();
          if (x<0 || x>=cairoSurface->width()) x=0;
          double y=clipBox[1]+0.5*cairoSurface->height();
          if (y<0 || y>=cairoSurface->height()) y=0;
          cairoSurface->blit
            (x, y, clipBox[2]-clipBox[0], clipBox[3]-clipBox[1]);
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
            opItem->op=&minsky::minsky.operations[tkMinskyItem->id];
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
            varItem->var=minsky::minsky.variables[tkMinskyItem->id];
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


//static int dum=registerItems();
static int dum=(initVec().push_back(registerItems), 0);

RenderVariable::RenderVariable(const VariablePtr& var, cairo_t* cairo):
  var(var), cairo(cairo)
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
  cairo_text_extents(lcairo,var->name.c_str(),&bbox);
  w=0.5*bbox.width+2; 
  h=0.5*bbox.height+2;

  if (surf) //cleanup temporary surface
    {
      cairo_destroy(lcairo);
      cairo_surface_destroy(surf);
    }
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
      cairo_show_text(cairo,var->name.c_str());
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
  
      // calculate port coordinates in current rotation
      double x0=w+2, y0=0, x1=-w+2, y1=0;
      cairo_save(cairo);
      cairo_identity_matrix(cairo);
      cairo_rotate(cairo, angle);
      cairo_user_to_device(cairo, &x0, &y0);
      cairo_user_to_device(cairo, &x1, &y1);
      cairo_restore(cairo);

      // set the ports coordinates
      portManager().movePortTo(var->outPort(), var->x+x0, var->y+y0);
      portManager().movePortTo(var->inPort(), var->x+x1, var->y+y1);
    }
}

bool RenderVariable::inImage(float x, float y)
{
  float dx=x-var->x, dy=y-var->y;
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
  //  cairo_scale(cairo,1,1);
  cairo_move_to(cairo,10,0);
  cairo_line_to(cairo,0,-3);
  cairo_line_to(cairo,0,3);
  cairo_fill(cairo);
  cairo_restore(cairo);
}

