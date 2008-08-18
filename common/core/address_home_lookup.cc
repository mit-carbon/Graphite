/* for debugging -- remove this eventually */
#include <iostream> 
using namespace std; 

#include "address_home_lookup.h"


AddressHomeLookup::AddressHomeLookup(UINT32 total_num_cache_lines_arg, UINT32 num_nodes_arg, UINT32 bytes_per_cache_line_arg)
{
  total_num_cache_lines = total_num_cache_lines_arg;
  num_nodes = num_nodes_arg;
  
  cout << "total num cache lines: " << total_num_cache_lines << endl;
  cout << "Num_Nodes: " << num_nodes << endl;

  cache_lines_per_node = total_num_cache_lines / num_nodes;
  bytes_per_cache_line = bytes_per_cache_line_arg;

  assert(total_num_cache_lines % num_nodes == 0); // make sure each node has the same number of lines, and that there are no orphan lines
  
  /* we may want to handle this case in the future. for now, we assume easy memory allocation */
  if(total_num_cache_lines % num_nodes != 0)
    {
      throw "Memory size must be evenly divisible by the number of nodes.";
    }
  
  /*
    cout << "Initializing Address Home Lookup" << endl;
    cout << "total memory size: " << total_mem_size_bytes << " bytes" << endl;
    cout << "num nodes: " << num_nodes << endl;
  */
}

AddressHomeLookup::~AddressHomeLookup()
{
}

// TODO: proper return type -- node number

/*
 * Given an address, returns the home node number where the address's dram directory lives
 */
UINT32 AddressHomeLookup::find_home_for_addr(ADDRINT address) const {
  /* TODO: error check bound */
  
  /* TODO: something more sophisticated? */
  
  /* Memory Allocation Scheme:
   *   each node, of which there are num_nodes of them, gets an equally-sized
   *   statically-defined segment of memory.
   */
  

  // TODO: check this index arithmetic

//BUG: bytes_per_cache_line is 32 bytes, not 64
#ifdef SMEM_DEBUG
	cout << " AHL: find_home_for_address  =" << address << endl;
	printf(" AHL: find_home_for_address  = %x\n" ,address);
	cout << " AHL: bytes_per_cache_line   = " << bytes_per_cache_line << endl;
	cout << " AHL: cache_lines_per_node   = " << cache_lines_per_node << endl;
#endif

	
  int dram_line = address / bytes_per_cache_line;
  UINT32 return_core_id = dram_line / cache_lines_per_node;

	assert( return_core_id < num_nodes);
	return return_core_id;
//	return 0; //force it to home DRAM on core #1 TODO
}
