#include "utils.h"


/* ================================================================ */
/* Utility function definitions */
/* ================================================================ */

string myDecStr(UINT64 v, UINT32 w)
{
    ostringstream o;
    o.width(w);
    o << v;
    string str(o.str());
    return str;
}


bool isPower2(UINT32 n)
{ return ((n & (n - 1)) == 0); }


INT32 floorLog2(UINT32 n)
{
   INT32 p = 0;

   if (n == 0) return -1;

   if (n & 0xffff0000) { p += 16; n >>= 16; }
   if (n & 0x0000ff00) { p +=  8; n >>=  8; }
   if (n & 0x000000f0) { p +=  4; n >>=  4; }
   if (n & 0x0000000c) { p +=  2; n >>=  2; }
   if (n & 0x00000002) { p +=  1; }

   return p;
}


INT32 ceilLog2(UINT32 n)
{ return floorLog2(n - 1) + 1; }


