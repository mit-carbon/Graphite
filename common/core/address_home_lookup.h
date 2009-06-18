#ifndef ADDRESSHOMELOOKUP_H_
#define ADDRESSHOMELOOKUP_H_

#include <cassert>
#include <iostream>
#include "fixed_types.h"
#include "config.h"

using namespace std;

/* 
 * TODO abstract MMU stuff to a configure file to allow
 * user to specify number of memory controllers, and
 * the address space that each is in charge of.  Default behavior:
 * AHL assumes that every core has a memory controller,
 * and that each shares an equal number of address spaces.
 *
 * Each core has an AHL, but they must keep their data consistent
 * regarding boundaries!
 *
 * Maybe allow the ability to have public and private memory space?
 */

class AddressHomeLookup
{
   public:
      AddressHomeLookup(core_id_t core_id);
      ~AddressHomeLookup();
      UInt32 find_home_for_addr(IntPtr address) const;

   private:
      UInt32 m_total_cores;
      UInt32 m_ahl_param;
      SInt32 m_core_id;
      UInt32 m_cache_line_size;
};


#endif /*ADDRESSHOMELOOKUP_H_*/
