#ifndef STR_H
#define STR_H
#include <string>
#include <sstream>
namespace minsky
{
  /// utility function to create a string representation of a numeric type
  template <class T> string str(T x) {
    ostringstream s;
    s<<x;
    return s.str();
  }
}
#endif
