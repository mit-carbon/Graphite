#ifndef ADDRESSHOMELOOKUP_H_
#define ADDRESSHOMELOOKUP_H_

#include "assert.h"
#include "pin.H"

class AddressHomeLookup
{
public:
  AddressHomeLookup(UINT32, UINT32, UINT32);
  virtual ~AddressHomeLookup();
  
  /* TODO: change this return type to a node number type */
  UINT32 find_home_for_addr(ADDRINT) const;
  
 private:
  unsigned int total_num_cache_lines;
  unsigned int num_nodes;
  unsigned int cache_lines_per_node;
  unsigned int bytes_per_cache_line;
};

#endif /*ADDRESSHOMELOOKUP_H_*/
