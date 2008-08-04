/* for debugging -- remove this eventually */
#include <iostream> 
using namespace std; 

#include "AddressHomeLookup.h"


AddressHomeLookup::AddressHomeLookup(unsigned int total_num_cache_lines_arg, unsigned int num_nodes_arg)
{
	total_num_cache_lines = total_num_cache_lines_arg;
	num_nodes = num_nodes_arg;
	cache_lines_per_node = total_num_cache_lines / num_nodes;

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

virtual AddressHomeLookup::~AddressHomeLookup()
{
}

// TODO: proper return type -- node number

/*
 * Given an address, returns the home node number where the address's dram directory lives
 */
unsigned int AddressHomeLookup::find_home_for_addr(unsigned int address) const {
	/* TODO: error check bound */
	
	/* TODO: something more sophisticated? */
	
	/* Memory Allocation Scheme:
	 *   each node, of which there are num_nodes of them, gets an equally-sized
	 *   statically-defined segment of memory.
	 */

	int dram_line = addr / ocache->getLineSize();
	return dram_line / cache_lines_per_node;
}
