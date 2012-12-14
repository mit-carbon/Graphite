#pragma once

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
   AddressHomeLookup(UInt32 ahl_param, vector<tile_id_t>& tile_list, UInt32 cache_line_size);
   ~AddressHomeLookup();
   tile_id_t getHome(IntPtr address) const;

private:
   UInt32 _ahl_param;
   vector<tile_id_t> _tile_list;
   UInt32 _total_modules;
   UInt32 _cache_line_size;
};
