#ifndef __UTIL_H__
#define __UTIL_H__

#include <cmath>

using namespace std;

#ifdef _WIN32
inline double log2(double x)
{
  static const double xxx = 1.0/log(2.0);
  return log(x)*xxx;
}
#endif

#endif

