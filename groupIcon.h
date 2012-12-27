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
#ifndef GROUPICON_H
#define GROUPICON_H

#include <vector>
#include <algorithm>
#include <string>
#include "wire.h"
#include "variable.h"

#include "classdesc_access.h"
#include <TCL_obj_base.h>
#include <arrays.h>

#include <cairo/cairo.h>

namespace minsky
{
  using namespace ecolab;
  class Minsky;

  class GroupIcon
  {
  public:
  private:
    CLASSDESC_ACCESS(GroupIcon);
    std::vector<int> m_operations;
    std::vector<int> m_variables;
    std::vector<int> m_wires;
    /// input and output port variables of this group
    std::vector<int> inVariables, outVariables;
    float m_x, m_y, m_zoomFactor; ///< icon position, amount of zoom
    float displayZoom; ///< zoom at which contents are displayed
    int id;

    friend struct SchemaHelper;

    /// add variable to one of the edge lists, connected to port \a
    /// port. If the operation results in addition wires being
    /// created, these are returned in \a additionalWires
    void addEdgeVariable(std::vector<int>& varVector, 
                         std::vector<Wire>& additionalWires, int port);

    void drawVar(cairo_t*, const VariablePtr&, float, float) const;

    /// return current scope's Minksy object
    static Minsky& minsky();

    /// see if any attached wires should also be moved into the group
    template <class S> void addAnyWires(const S& ports);
    /// remove any attached wires that belong to the group
    template <class S> void removeAnyWires(const S& ports);

  public:

    /// RAII set the minsky object to a different one for the current scope
    struct LocalMinsky
    {
      LocalMinsky(Minsky& m);
      ~LocalMinsky();
    };

    std::string name;
    float width, height; // size of icon
    float rotation; // orientation of icon
    std::vector<int> ports() const;
    int numPorts() const {return inVariables.size()+outVariables.size();}

    /// returns a list of edgeVariables
    std::vector<VariablePtr> edgeVariables() const;
    /// return the set of variable ideas, suitable for membership testing
    std::set<int> edgeSet() const {
      std::set<int> r(inVariables.begin(), inVariables.end());
      r.insert(outVariables.begin(), outVariables.end());
      return r;
    }

    const std::vector<int>& operations() const {return m_operations;}
    const std::vector<int>& variables() const {return m_variables;}
    const std::vector<int>& wires() const {return m_wires;}
    
    /// @{ coordinates of this icon on canvas
    float x() const {return m_x;}
    float y() const {return m_y;}
    /// @}

    /// x-coordinate of the vertical centre line of the icon
    float iconCentre() const {
      float left, right;
      margins(left,right);
      return x()+zoomFactor()*0.5*(left-right);
    }
                                   
    GroupIcon(int id=-1): m_x(0), m_y(0), m_zoomFactor(1), 
                          width(100), height(100), rotation(0), 
                          displayZoom(1), id(id) {}

    /// group all icons in rectangle bounded by (x0,y0):(x1,y1)
    void group(float x0, float y0, float x1, float y1);
    /// ungroup all icons
    void ungroup();

    /// populates this with a copy of src (with all internal objects
    /// registered with minsky).
    void copy(const GroupIcon& src);

    /// update port locations to current geometry and rotation.  Return
    /// the relative locations of the ports in unrotated coordinates as
    /// a vector of (x,y) pairs
    array<float> updatePortLocation();

    /// draw representations of edge variables around group icon
    void drawEdgeVariables(cairo_t*, float xScale, float yScale) const;

    /// margin sizes to allow space for edge variables. 
    void margins(float& left, float& right) const;

    void MoveTo(float x1, float y1); ///< absolute move
    void moveTo(TCL_args args) {MoveTo(args[0], args[1]);}

    /// return bounding box coordinates for all variables, operators
    /// etc in this group
    void contentBounds(float& x0, float& y0, float& x1, float& y1) const;

    /// zoom by \a factor, scaling all widget's coordinates, using (\a
    /// xOrigin, \a yOrigin) as the origin of the zoom transformation
    void zoom(float xOrigin, float yOrigin,float factor);
    void setZoom(float factor) {m_zoomFactor=factor;}
    float zoomFactor() const {return m_zoomFactor;}

    /// delete contents, leaving an empty group
    void deleteContents();

    /// computes the zoom at which to show contents, given current
    /// contentBounds and width
    float computeDisplayZoom();
    float localZoom() const {return zoomFactor()/displayZoom;}

    /// returns whether contents should be displayed
    bool displayContents() const {return zoomFactor()>displayZoom;}

    /// add a variable to to the group
    void AddVariable(int id);
    void addVariable(TCL_args args) {AddVariable(args);}
    /// remove variable from group
    void RemoveVariable(int id);
    void removeVariable(TCL_args args) {RemoveVariable(args);}
    /// add a operator to to the group
    void AddOperation(int id);
    void addOperation(TCL_args args) {AddOperation(args);}
    /// remove operator from group
    void RemoveOperation(int id);
    void removeOperation(TCL_args args) {RemoveOperation(args);}

    /// rotate icon and all its contents
    void Rotate(float angle);
    void rotate(TCL_args args) {Rotate(args);}
    /// rotate icon to a given rotation value
    void rotateTo(TCL_args args) {
      float angle=args;
      Rotate(angle-rotation);
    }
  };
}

#include "groupIcon.cd"
#endif
