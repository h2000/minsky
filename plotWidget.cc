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
#include "plotWidget.h"
#include "variable.h"
#include "init.h"
#include "cairoItems.h"
#include "minsky.h"
using namespace ecolab::cairo;
using namespace ecolab;
using namespace std;
using namespace minsky;

namespace
{
  const int numLines = 4; // number of simultaneous variables to plot

  map<string, shared_ptr<TkPhotoSurface> > surfaces;

  Plots::Map& plots() {return minsky::minsky.plots.plots;}

  /// temporarily sets nTicks and fontScale, restoring them on scope exit
  struct SetTicksAndFontSize
  {
    int nxTicks, nyTicks;
    double fontScale;
    bool grid;
    PlotWidget& p;
    SetTicksAndFontSize(PlotWidget& p, bool override, int n, double f, bool g):
      p(p), nxTicks(p.nxTicks), nyTicks(p.nyTicks), 
      fontScale(p.fontScale), grid(p.grid) 
    {
      if (override)
        {
          p.nxTicks=p.nyTicks=n;
          p.fontScale=f;
          p.grid=g;
        }
    }
    ~SetTicksAndFontSize()
    {
      p.nxTicks=nxTicks;
      p.nyTicks=nyTicks;
      p.fontScale=fontScale;
      p.grid=grid;
    }
  };

  struct PlotItem: public cairo::CairoImage
  {
    PlotWidget& pw;
    string image;
    PlotItem(const std::string& image): 
      CairoImage(image), pw(plots()[image]), image(image)  {
      surfaces[image]=cairoSurface;
      if (cairoSurface && pw.images.empty())
        {      
          pw.images.push_back(image);
          float w=cairoSurface->width(), h=cairoSurface->height();
          float x = -0.5*w, dx=w/numLines; // x location of ports
          float y=0.5*h, dy = h/(numLines);

          pw.x=0; pw.y=0; // set up everything relative to pw centroid

          // xmin, xmax, ymin, ymax ports
          pw.ports<<=portManager().addPort(Port(-0.46*w,0.49*h,true)); //xmin
          pw.ports<<=portManager().addPort(Port(0.49*w,0.49*h,true));  //xmax
          pw.ports<<=portManager().addPort(Port(-0.49*w,0.47*h,true)); //ymin
          pw.ports<<=portManager().addPort(Port(-0.49*w,-0.49*h,true)); //ymax

          // y variable ports
          for (float y=0.5*(dy-h); y<0.5*h; y+=dy)
            pw.ports<<=portManager().addPort(Port(x,y,true));

          // add in the x variable ports
          for (float x=0.5*(dx-w); x<0.5*w; x+=dx)
             pw.ports<<=portManager().addPort(Port(x,y,true));

        }
      pw.nxTicks=pw.nyTicks=10;
      pw.fontScale=1;
      pw.offx=10; pw.offy=5;
      pw.leadingMarker=true;
      pw.grid=true;
      pw.Image(image);
    }
//    ~PlotItem() {
//      plots().erase(image);
//    }
    void draw();
  };

  void PlotItem::draw()
  {
    if (cairoSurface)
      {
        cairoSurface->clear();
        cairo_t* cairo=cairoSurface->cairo();
        double w=cairoSurface->width(), h=cairoSurface->height();

        //        cairo_save(cairo);
        cairo_identity_matrix(cairo);

        cairo_reset_clip(cairo);
        cairo_new_path(cairo);
        cairo_rectangle(cairo,0,0,w,h);
        cairo_clip(cairo);

        // draw bounding box ports
        static const double orient[]={-0.4*M_PI, -0.6*M_PI, -0.2*M_PI, 0.2*M_PI};
        for (size_t i=0; i<4; ++i)
          drawTriangle(cairo, portManager().ports[pw.ports[i]].x-pw.x+0.5*w, 
                       portManager().ports[pw.ports[i]].y-pw.y+0.5*h, 
                       palette[(i/2)%paletteSz],
                       orient[i]);

        // draw data ports
        for (size_t i=4; i<pw.ports.size(); ++i)
          drawTriangle(cairo, portManager().ports[pw.ports[i]].x-pw.x+0.5*w, 
                       portManager().ports[pw.ports[i]].y-pw.y+0.5*h, 
                       palette[((i-4)%numLines)%paletteSz],
                       (i<numLines+4)? 0: -0.5*M_PI);


        SetTicksAndFontSize stf(pw, true, 3, 3, false);
        pw.scalePlot();
        pw.draw(*cairoSurface);

        cairoSurface->blit();
        //        cairo_restore(cairo);
      }
  }

#ifdef __GNUC__
#pragma GCC push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
  // register PlotItem with Tk for use in canvases.
  int registerPlotItem()
  {
    static Tk_ItemType plotItemType = cairoItemType;
    plotItemType.name="plot";
    plotItemType.createProc=createImage<PlotItem>;
    Tk_CreateItemType(&plotItemType);
    return 0;
  }
#ifdef __GNUC__
#pragma GCC pop
#endif

}

//static int reg=registerPlotItem();
static int reg=(initVec().push_back(registerPlotItem), 0);

/// TODO: better handled in a destructor, but for the moment, the
/// software architecture requires that this be called explicitly from
/// Minsky::deletePlot
void PlotWidget::deletePorts()
{
  for (size_t p=0; p<ports.size(); ++p)
    portManager().delPort(ports[p]);
  ports.resize(0);
}

void PlotWidget::MoveTo(float x1, float y1)
{
  float w=width(), h=height();
  float dx=x1-x, dy=y1-y;
  x=x1; y=y1;
  for (size_t i=0; i<ports.size(); ++i)
    portManager().movePort(ports[i], dx, dy);
}

void PlotWidget::scalePlot()
{
  // set any scale overrides
  setMinMax();
  if (xminVar.idx()>-1) {minx=xminVar.value();}
  if (xmaxVar.idx()>-1) {maxx=xmaxVar.value();}
  if (yminVar.idx()>-1) {miny=yminVar.value();}
  if (ymaxVar.idx()>-1) {maxy=ymaxVar.value();}
  autoscale=false;
}

void PlotWidget::redraw()
{
  scalePlot();
  for (size_t i=0; i<images.size(); ++i)
    {
      map<string, shared_ptr<TkPhotoSurface> >::iterator surf=
        surfaces.find(images[i]);
      if (surf!=surfaces.end())
        {
          SetTicksAndFontSize stf(*this, i==0, 3, 3, false);
          surf->second->clear();
          draw(*surf->second);
          surf->second->blit();
        }
    }
}

void PlotWidget::addPlotPt(double t)
{
  scalePlot();
  
  for (size_t i=0; i<images.size(); ++i)
    {
      map<string, shared_ptr<TkPhotoSurface> >::iterator surf=
        surfaces.find(images[i]);
      if (surf!=surfaces.end())
        {
          SetTicksAndFontSize stf(*this, i==0, 3, 3, false);
          double x[yvars.size()], y[yvars.size()];
          for (size_t pen=0; pen<yvars.size(); ++pen)
            if (yvars[pen].idx()>=0)
              {
                switch (xvars.size())
                  {
                  case 0: // use t, when x variable not attached
                    x[pen]=t;
                    y[pen]=yvars[pen].value();
                    break;
                  case 1: // use the value of attached variable
                    assert(xvars[0].idx()>=0);
                    x[pen]=xvars[0].value();
                    y[pen]=yvars[pen].value();
                    break;
                  default:
                    if (pen < xvars.size() && xvars[pen].idx()>=0)
                      {
                        x[pen]=xvars[pen].value();
                        y[pen]=yvars[pen].value();
                      }
                    else
                      throw error("x input not wired for pen %d",(int)pen+1);
                    break;
                  }
                add(*surf->second, pen, x[pen], y[pen]);
              }
                  
          //TODO: I honestly do not know why these lines need to be here.
          surf->second->clear();

          // reset grid drawing on canvas plots
          draw(*surf->second);
          surf->second->blit();
          
        }
    }
}

static VariableValue disconnected;

void PlotWidget::connectVar(const VariableValue& var, unsigned port)
{
  if (port<4)
    switch (port)
      {
      case 0: xminVar=var; return;
      case 1: xmaxVar=var; return;
      case 2: yminVar=var; return;
      case 3: ymaxVar=var; return;
      }
  unsigned pen=port-4;
  if (pen<numLines)
    {
      yvars.resize(pen+1);
      yvars[pen]=var;
    }
  else if (pen<2*numLines)
    {
      xvars.resize(pen-numLines+1);
      xvars[pen-numLines]=var;
    }
}

//void Plots::moveTo(TCL_args args)
//{
//  plots[args[0]].MoveTo(args[1],args[2]);
//}

void Plots::addImage(TCL_args args) {
  string id=args, image=args;
  surfaces[image].reset
    (new TkPhotoSurface(Tk_FindPhoto(interp(), image.c_str()), false));
  PlotWidget& p=plots[id];
  // only insert image if not already present
  if (find(p.images.begin(), p.images.end(), image)==p.images.end())
    p.images.push_back(image);
  TkPhotoSurface& s=*surfaces[image];
  s.clear();
  p.draw(s);
  s.blit();
}

void Plots::reset(const VariableManager& vm)
{
  for (Map::iterator p=plots.begin(); p!=plots.end(); ++p)
    p->second.clear();
}

string Plots::nextPlotID()
{
  int id=0;
  if (!plots.empty())
    {
      string lastID=plots.rbegin()->first;
      sscanf(lastID.c_str(),"plot_image%d",&id);
      id++;
    }
  ostringstream os;
  os << "plot_image"<<id;
  return os.str();
}
