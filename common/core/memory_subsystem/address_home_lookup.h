#ifndef __ADDRESS_HOME_LOOKUP_H__
#define __ADDRESS_HOME_LOOKUP_H__

#include <vector>
using namespace std;

#include "fixed_types.h"

/* 
 * TODO abstract MMU stuff to a configure file to allow
 * user to specify number of memory controllers, and
 * the address space that each is in charge of.  Default behavior:
 * AHL assumes that every tile has a memory controller,
 * and that each shares an equal number of address spaces.
 *
 * Each tile has an AHL, but they must keep their data consistent
 * regarding boundaries!
 *
 * Maybe allow the ability to have public and private memory space?
 */

class AddressHomeLookup
{
   public:
      AddressHomeLookup(UInt32 ahl_param,
            vector<tile_id_t>& tile_list,
            UInt32 cache_block_size);
      ~AddressHomeLookup();
      tile_id_t getHome(IntPtr address) const;

   private:
      UInt32 m_ahl_param;
      vector<tile_id_t> m_tile_list;
      UInt32 m_total_modules;
      UInt32 m_cache_block_size;
};

#endif /* __ADDRESS_HOME_LOOKUP_H__ */
