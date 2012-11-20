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
#include "variable.h"
#include <plot.h>
#include <cairo/cairo.h>

namespace minsky
{
  /** class that randers a variable into a cairo context. 
      A user can also query the size of the unrotated rendered image
  */
  class RenderVariable
  {
    const VariablePtr& var;
    cairo_t *cairo;
    float w, h;
    double xScale, yScale;
  public:
    // render a variable to a given cairo context
    RenderVariable(const VariablePtr& var, cairo_t* cairo=NULL, 
                   double xScale=1, double yScale=1);
    /// render the cairo image
    void draw();
    /// half width of unrotated image
    float width() const {return w;}
    /// half height of unrotated image
    float height() const {return h;}
    bool inImage(float x, float y); // true if variable within rendered image
  };

  void drawTriangle(cairo_t* cairo, double x, double y, cairo::Colour& col, double angle=0);

}
