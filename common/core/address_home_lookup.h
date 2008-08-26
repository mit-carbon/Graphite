#ifndef ADDRESSHOMELOOKUP_H_
#define ADDRESSHOMELOOKUP_H_

#include "assert.h"
#include "pin.H"

/* TODO abstract MMU stuff to a configure file to allow
 * user to specify number of memory controllers, and
 * the address space that each is in charge of.  Currently,
 * AHL assumes that every core has a memory controller,
 * and that each shares an equal number of address spaces.
 */

class AddressHomeLookup
{
public:
  AddressHomeLookup(UINT32, UINT32, UINT32);
  virtual ~AddressHomeLookup();
  
  /* TODO: change this return type to a node number type */
  UINT32 find_home_for_addr(ADDRINT) const;
  
 private:
  UINT32 total_num_cache_lines;
  UINT32 num_nodes;
  UINT32 cache_lines_per_node;
  UINT32 bytes_per_cache_line;
};

#endif /*ADDRESSHOMELOOKUP_H_*/
