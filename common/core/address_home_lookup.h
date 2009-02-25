#ifndef ADDRESSHOMELOOKUP_H_
#define ADDRESSHOMELOOKUP_H_

#include "assert.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <math.h>
#include "fixed_types.h"

using namespace std;

/* TODO abstract MMU stuff to a configure file to allow
 * user to specify number of memory controllers, and
 * the address space that each is in charge of.  Default behavior:
 * AHL assumes that every core has a memory controller,
 * and that each shares an equal number of address spaces.
 *
 * Now possible to setAddrBoundaries, either through constructor
 * or during runtime!
 *
 * each core has an AHL, but they must keep their data consistent
 * regarding boundaries!
 *
 * Maybe allow the ability to have public and private memory space?
 */

class AddressHomeLookup
{
   public:
      AddressHomeLookup(UInt32 num_nodes, UInt32 log_block_size, SInt32 ahl_id, UInt32 line_size);
      ~AddressHomeLookup();
      UInt32 find_home_for_addr(IntPtr address) const;

   private:
      UInt32 num_nodes;
      SInt32 ahl_id;
      UInt32 log_block_size;
      UInt32 line_size;
};


#endif /*ADDRESSHOMELOOKUP_H_*/
