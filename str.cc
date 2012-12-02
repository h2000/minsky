#include "str.h"
#include <sstream>

namespace minsky
{

  string str(long x)
  {
    ostringstream o;
    o<<x;
    return o.str();
  }

  string str(double x)
  {
    ostringstream o;
    o<<x;
    return o.str();
  }
}
