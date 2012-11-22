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

#include "init.h"
#include "XGLItem.h"
#include <xgl/cairorenderer.h>
#include <xgl/xgl.h>
#include <cairo_base.h>
#include <arrays.h>
using namespace ecolab::cairo;
using namespace ecolab;
using namespace std;
using namespace minsky;

namespace 
{
  // we need some extra fields to handle the additional options
  struct TkXGLItem: public ImageItem
  {
    int id; // C++ object identifier
    char* xglRes; // name of the XGL file to display    
  };

  static Tk_CustomOption tagsOption = {
    (Tk_OptionParseProc *) Tk_CanvasTagsParseProc,
    Tk_CanvasTagsPrintProc, (ClientData) NULL
  };


  int xglCreatProc(Tcl_Interp *interp, Tk_Canvas canvas, 
                         Tk_Item *itemPtr, int objc,Tcl_Obj *CONST objv[])
  {
    TkXGLItem* tkXGLItem=(TkXGLItem*)(itemPtr);
    tkXGLItem->id=-1;
    tkXGLItem->xglRes=NULL;
    int r=createImage<XGLItem>(interp,canvas,itemPtr,objc,objv);
    if (r==TCL_OK)
      {
        XGLItem* xglItem=(XGLItem*)(tkXGLItem->cairoItem);
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

  // overrride cairoItem's configureProc to process the extra config options
  int configureProc(Tcl_Interp *interp,Tk_Canvas canvas,Tk_Item *itemPtr,
                    int objc,Tcl_Obj *CONST objv[],int flags)
  {
    return TkImageCode::configureCairoItem
      (interp,canvas,itemPtr,objc,objv,flags, XGLItem::configSpecs);
  }

}

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wwrite-strings"
// causes problems on MacOSX
//#undef Tk_Offset
//#define Tk_Offset(type, field) ((int) ((char *) &((type *) 1)->field)-1)
#endif
Tk_ItemType& minsky::xglItemType()
{
  static Tk_ItemType xglItemType = cairoItemType;
  xglItemType.name="xgl";
  xglItemType.itemSize=sizeof(TkXGLItem);
  xglItemType.createProc=xglCreatProc;
  xglItemType.configProc=configureProc;
  xglItemType.configSpecs=XGLItem::configSpecs;
  Tk_CreateItemType(&xglItemType);
  return xglItemType;
}

static int registerItems()
{minsky::xglItemType(); return 0;}

//static int dum=registerItems();
static int dum=(initVec().push_back(registerItems), 0);

void XGLItem::draw()
{
  if (cairoSurface)
    {
      CairoRenderer renderer(cairoSurface->surface());
      xgl drawing(renderer);
      drawing.load(xglRes.c_str());
      drawing.render();
      array<double> bbox=boundingBox();
      cairoSurface->blit
        (bbox[0]+0.5*cairoSurface->width(), 
         bbox[1]+0.5*cairoSurface->height(), 
         bbox[2]-bbox[0], bbox[3]-bbox[1]);
    }
}

Tk_ConfigSpec XGLItem::configSpecs[] =
  {
    {TK_CONFIG_STRING, "-image", NULL, NULL,
     NULL, Tk_Offset(ImageItem, imageString), 0},
    {TK_CONFIG_DOUBLE, "-scale", NULL, NULL,
     "1.0", Tk_Offset(ImageItem, scale), TK_CONFIG_NULL_OK},
    {TK_CONFIG_DOUBLE, "-rotation", NULL, NULL,
     "0.0", Tk_Offset(ImageItem, rotation), TK_CONFIG_NULL_OK},
    {TK_CONFIG_INT, "-id", NULL, NULL,
     NULL, Tk_Offset(TkXGLItem, id), 0},
    {TK_CONFIG_STRING, "-xgl", NULL, NULL,
     NULL, Tk_Offset(TkXGLItem, xglRes), 0},
    {TK_CONFIG_CUSTOM, "-tags", NULL, NULL,
     NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
    {TK_CONFIG_END}
  };



