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
#include "operation.h"

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
    std::vector<int> m_groups;
    /// input and output port variables of this group
    std::set<int> inVariables, outVariables;
    /// used for ensuring that only one reference to a given variable
    /// is included
    std::set<std::string> inVarNames, outVarNames;
    float m_x, m_y; ///< icon position
    float m_localZoom;
    int id, m_parent;

    friend struct SchemaHelper;

    /// add variable to one of the edge lists, connected to port \a
    /// port. If the operation results in addition wires being
    /// created, these are returned in \a additionalWires
    void addEdgeVariable(std::set<int>& vars, std::set<string>& varNames,
                         std::vector<Wire>& additionalWires, int port);

    void drawVar(cairo_t*, const VariablePtr&, float, float) const;

  public:

    std::string name;
    float width, height; // size of icon
    float rotation; // orientation of icon
    bool visible;
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
    /// eliminate any duplicate I/O variable references
    void eliminateIOduplicates();

    const std::vector<int>& operations() const {return m_operations;}
    const std::vector<int>& variables() const {return m_variables;}
    const std::vector<int>& wires() const {return m_wires;}
    const std::vector<int>& groups() const {return m_groups;}
    
    /// @{ coordinates of this icon on canvas
    float x() const;
    float y() const;
    /// @}

    int parent() const {return m_parent;}
    /// synonym for parent(), for TCL scripting purposes
    int group() const {return parent();}

    /// @return true if gid is a parent, or parent of a parent, etc
    bool isAncestor(int gid) const;

    /// x-coordinate of the vertical centre line of the icon
    float iconCentre() const {
      float left, right;
      margins(left,right);
      return x()+zoomFactor*0.5*(left-right);
    }

    // scaling factor to allow a rotated icon to fit on the bitmap
    float rotFactor() const;
                                   
    GroupIcon(int id=-1): m_x(0), m_y(0), zoomFactor(1), m_localZoom(1),
                          width(100), height(100), rotation(0), visible(true),
                          displayZoom(1), id(id), m_parent(-1) {}

    /// group all icons in rectangle bounded by (x0,y0):(x1,y1)
    void createGroup(float x0, float y0, float x1, float y1);
    /// ungroup all icons
    void ungroup();

    bool empty() {return m_variables.empty() && m_operations.empty() && 
        m_wires.empty() && m_groups.empty();}

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
    void move(float dx, float dy) {MoveTo(x()+dx, y()+dy);}

    /// return bounding box coordinates for all variables, operators
    /// etc in this group
    void contentBounds(float& x0, float& y0, float& x1, float& y1) const;

    /// for TCL debugging
    array<float> cBounds() const {
      array<float> r(4);
      contentBounds(r[0],r[1],r[2],r[3]);
      return r;
    }

    /// zoom by \a factor, scaling all widget's coordinates, using (\a
    /// xOrigin, \a yOrigin) as the origin of the zoom transformation
    void zoom(float xOrigin, float yOrigin,float factor);
    float zoomFactor;
    /// sets the zoomFactor, and the appropriate zoom factors for all
    /// contained items
    void setZoom(float factor);

    /// delete contents, leaving an empty group
    void deleteContents();

    /// computes the zoom at which to show contents, given current
    /// contentBounds and width
    float displayZoom; ///< zoom at which contents are displayed
    float computeDisplayZoom();
    //    float localZoom() const {return m_localZoom;}
    float localZoom() const {
      return (displayZoom>0 && zoomFactor>displayZoom)
        ? zoomFactor/displayZoom: 1;
    }

    /// returns 1 if x,y is located in the in margin, 2 if in the out
    /// margin, 0 otherwise
    int InIORegion(float x, float y) const;
    int inIORegion(TCL_args args) const {return InIORegion(args[0], args[1]);}

    /// returns whether contents should be displayed
    bool displayContents() const {return zoomFactor>displayZoom;}

    /// add a variable to to the group
    void addVariable(std::pair<const int,VariablePtr>&);
    /// remove variable from group
    void removeVariable(std::pair<const int,VariablePtr>&);
    /// add a operation to to the group
    void addOperation(std::pair<const int,OperationPtr>&);
    /// remove operation from group
    void removeOperation(std::pair<const int,OperationPtr>&);
    /// make group a child of this group
    /// @return true if successful
    bool addGroup(std::pair<const int,GroupIcon>&);
    /// remove child group from this group, and add it to the parent
    void removeGroup(std::pair<const int,GroupIcon>&);
    
    /// see if any attached wires should also be moved into the group
    /// \a ports is a sequence (vector or array) of ports
    template <class S> void addAnyWires(const S& ports);
    /// remove any attached wires that belong to the group
    template <class S> void removeAnyWires(const S& ports);


    /// rotate icon and all its contents
    void Rotate(float angle);
    void rotate(TCL_args args) {Rotate(args);}
    /// rotate icon to a given rotation value
    void rotateTo(TCL_args args) {
      float angle=args;
      Rotate(angle-rotation);
    }

    /// returns the name of a variable if point (x,y) is within a
    /// variable icon, "@" otherwise, indicating that the Godley table
    /// has been selected.
    int Select(float x, float y) const;
    int select(TCL_args args) const {return Select(args[0],args[1]);}


  };

  struct GroupIcons: public std::map<int, GroupIcon>
  {
    std::vector<int> visibleGroups() const;
  };

}

#include "groupIcon.cd"
#endif
