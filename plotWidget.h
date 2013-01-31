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
#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H
#include <cairo_base.h>
#include <TCL_obj_base.h>
#include "classdesc_access.h"
#include "plot.h"
#include "variable.h"
#include "portManager.h"
#include "variableManager.h"
#include "zoom.h"

#include "port.h"

namespace minsky
{
  // a container item for a plot widget
  class PlotWidget: public ecolab::Plot
  {
    float m_x, m_y;
    CLASSDESC_ACCESS(PlotWidget);
    friend class SchemaHelper;
  public:
    array<int> ports;
    void deletePorts();

    /// variable port attached to (if any)
    std::vector<VariableValue> yvars;
    std::vector<VariableValue> xvars;

    /// variable ports specifying plot size
    VariableValue xminVar, xmaxVar, yminVar, ymaxVar;
    /// number of ticks to show in canvas item
    unsigned displayNTicks;
    double displayFontSize;

    std::vector<string> images;
 
    /// @{ coordinates of this plot widget on the canvas
    float x() const {return m_x;}
    float y() const {return m_y;}
    /// @}

    PlotWidget(): m_x(0), m_y(0), zoomFactor(1), 
                  displayNTicks(3), displayFontSize(3) {grid=true;}

    void MoveTo(float x, float y);
    void moveTo(TCL_args args) {MoveTo(args[0],args[1]);}
    void addPlotPt(double t); ///< add another plot point
    /// connect variable \a var to port \a port. 
    void connectVar(const VariableValue& var, unsigned port);
    void redraw(); // redraw plot using current data

    /// set autoscaling
    void autoScale() {xminVar=xmaxVar=yminVar=ymaxVar=VariableValue();}
    void scalePlot();

    /// adjust coordinates and zoomFactor, where (\a xOrigin, \a
    /// yOrigin) is the origin of the zooming
    void zoom(float xOrigin, float yOrigin, float factor) {
      minsky::zoom(m_x, xOrigin, factor);
      minsky::zoom(m_y, yOrigin, factor);
      zoomFactor*=factor;
    }
    float zoomFactor;
  };

  /// global register of plot widgets, indexed by the item image name
  struct Plots
  {
    typedef std::map<string, PlotWidget> Map;
    Map plots;
    /// add an addition image surface to a PlotWidget
    /// @param id - image name identifying the plotWidget
    /// @param image - image name for new surface to be added
    void addImage(TCL_args args);
    /// move a plotwidget to x y on the canvas
    /// @param id - image name identifying plotwidget to move
    /// @param x - x coordinate
    /// @paral y - y coordinate
//    void moveTo(TCL_args args);
//    float X(TCL_args args) {return plots[args].x;}
//    float Y(TCL_args args) {return plots[args].y;}
//    void ports(TCL_args args) {
//      PlotWidget& pw=plots[args];
//      tclreturn ret;
//      for (size_t p=0; p<pw.ports.size(); ++p)
//        ret<<pw.ports[p];
//    }
    /// reset the plots. Needs to be called after a VariableManager has been reset
    void reset(const VariableManager&);
    /// returns a suitable image identifier for the next plot to be added
    string nextPlotID();
    void clear() {plots.clear();}
  };
}

#ifdef CLASSDESC
#pragma omit pack PlotItem
#pragma omit unpack PlotItem
#endif

inline void xml_pack(classdesc::xml_pack_t&,const string&,Plot&) {}
inline void xml_unpack(classdesc::xml_unpack_t&,const string&,Plot&) {}

#include "plotWidget.cd"
#endif
