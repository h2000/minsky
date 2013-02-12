/*
  @copyright Steve Keen 2013
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

/// some useful geometry types, defined from boost::geometry

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <boost/geometry/geometries/point_xy.hpp>
//#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/assign.hpp>

namespace minsky
{
  typedef boost::geometry::model::d2::point_xy<float> Point;
  typedef boost::geometry::model::ring<Point> Polygon;
  typedef boost::geometry::model::box<Point> Rectangle;

  /// convenience method for using += and , to assign multiple points
  /// to a Polygon
  inline boost::assign::list_inserter
  < boost::assign_detail::call_push_back< Polygon >, Point > 
  operator+=( Polygon& c, Point v )
  {
    return boost::assign::make_list_inserter
      (boost::assign_detail::call_push_back<Polygon>(c))(v);
  }

  /// rotate (x,y) by \a rot (in degrees) around the origin \a (x0, y0)
  /// can be used for rotating multiple points once constructed
  class Rotate
  {
    float angle; // in radians
    float ca, sa;
    float x0, y0;
  public:
    Rotate(float rot, float x0, float y0): 
      angle(rot*M_PI/180.0), ca(cos(angle)), sa(sin(angle)), x0(x0), y0(y0) {}
    /// rotate (x,y) 
    Point operator()(float x1, float y1) const {
      return Point(x(x1,y1),y(x1,y1));}
    Point operator()(const Point& p) const {return operator()(p.x(), p.y());}
    float x(float x, float y) const {return ca*(x-x0)-sa*(y-y0)+x0;}
    float y(float x, float y) const {return sa*(x-x0)+ca*(y-y0)+y0;}
  };


}

#endif
