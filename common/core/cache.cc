#include "cache.h"

UINT32 CacheBase::rand_counter;

/* ================================================================================================ */
/* CacheBase methods */
/* ================================================================================================ */

CacheBase::CacheBase(std::string name, UINT32 size, UINT32 line_bytes, UINT32 assoc) : 
   name(name), cache_size(size), line_size(line_bytes), associativity(assoc),
   line_shift(floorLog2(line_bytes)), set_index_mask((size / (assoc * line_bytes)) - 1)
{

   assert(isPower2(line_size));
   assert(isPower2(set_index_mask + 1));

   for (UINT32 access_type = 0; access_type < k_ACCESS_TYPE_NUM; access_type++)
   {
      access[access_type][false] = 0;
      access[access_type][true] = 0;
   }
}


// Stats output method
#ifndef PERFORMING_CACHE_TEST
// FIXME: the above is a hack to fix a compilation error. I guess ljstr, fltstr are in a Pin library
// We should remove the dependency on Pin
string CacheBase::statsLong(string prefix, CacheType cache_type) const
{
   const UINT32 header_width = 19;
   const UINT32 number_width = 12;

   string out;
    
   out += prefix + name + ":" + "\n";

   if (cache_type != k_CACHE_TYPE_ICACHE) {
      for (UINT32 i = 0; i < k_ACCESS_TYPE_NUM; i++)
      {
         const AccessType access_type = AccessType(i);

         std::string type(access_type == k_ACCESS_TYPE_LOAD ? "Load" : "Store");

         out += prefix + ljstr(type + "-Hits:      ", header_width)
             + myDecStr(getHits(access_type), number_width)
             + "  " +fltstr(100.0 * getHits(access_type) / safeFDiv(getAccesses(access_type)), 2, 6) 
             + "%\n";

         out += prefix + ljstr(type + "-Misses:    ", header_width)
             + myDecStr(getMisses(access_type), number_width)
             + "  " +fltstr(100.0 * getMisses(access_type) / safeFDiv(getAccesses(access_type)), 2, 6) 
             + "%\n";
        
         out += prefix + ljstr(type + "-Accesses:  ", header_width)
             + myDecStr(getAccesses(access_type), number_width)
             + "  " +fltstr(100.0 * getAccesses(access_type) / safeFDiv(getAccesses(access_type)), 2, 6) 
             + "%\n";
        
         out += prefix + "\n";
      }
   }

   out += prefix + ljstr("Total-Hits:      ", header_width)
       + myDecStr(getHits(), number_width)
       + "  " +fltstr(100.0 * getHits() / getAccesses(), 2, 6) + "%\n";

   out += prefix + ljstr("Total-Misses:    ", header_width)
       + myDecStr(getMisses(), number_width)
       + "  " +fltstr(100.0 * getMisses() / getAccesses(), 2, 6) + "%\n";

   out += prefix + ljstr("Total-Accesses:  ", header_width)
       + myDecStr(getAccesses(), number_width)
       + "  " +fltstr(100.0 * getAccesses() / getAccesses(), 2, 6) + "%\n";

   out += "\n";

   return out;
}

#endif
