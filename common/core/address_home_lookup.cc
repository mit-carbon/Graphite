#include "address_home_lookup.h"

AddressHomeLookup::AddressHomeLookup (UINT32 num_nodes_arg, UINT32 log_block_size_arg, INT32 ahl_id_arg) {

	// Find number of directory cache banks
	// Let them be 2^k (so, we just need 'k' bits of input to differentiate between them)
	//
	// Lets say you get 'N' bit addresses
	// Divide the address into 3 portions as shown below
	// ***************************************
	// |   N-m-k     |    k    |     m       |
	// ***************************************
	//
	// Here, we just build a mask so that given any address, we can just do a bitwise 'AND' followed 
	// by a shift to the right by 'm' bits to get the home node number 
  
	// Inputs:
	//   m, k
	// m = log(block_size_arg)
	// k = log(core_count)
	
	UINT32 k = 0;

	// Now, num_nodes must be a power of 2
	ahl_id = ahl_id_arg;
	num_nodes = num_nodes_arg;
	
	//  k = log(num_nodes) to the base 2
	
	//  make sure that the block boundaries coincide with cache_line boundaries
	//TODO extern the cache_line_size knob
	assert ( ( (1 << log_block_size_arg) % g_knob_line_size ) == 0 );	
	assert (num_nodes > 0);
	
	while (!(num_nodes_arg & 0x1)) {
		k++;
		num_nodes_arg = num_nodes_arg >> 1;
	}
	assert (k > 0);
	
	log_block_size = log_block_size_arg;
	assert (log_block_size > 0);
	mask = ((1u << k) - 1) << log_block_size;

}

UINT32 AddressHomeLookup::find_home_for_addr(ADDRINT address) const {

	UINT32 node = (address & mask) >> log_block_size;
	assert (0 <= node && node <= num_nodes);
	return (node);

}

AddressHomeLookup::~AddressHomeLookup() {
	// TODO: Some meaningful scheme
}

/*

AddressHomeLookup::AddressHomeLookup(UINT32 num_nodes_arg, INT32 ahl_id_arg)
{
	num_nodes = num_nodes_arg;
  	ahl_id = ahl_id_arg;

	// default: divide up dram memory evenly between all of the cores 
	for(int i = 0; i < (int) num_nodes; i++)
	{
		//TODO: verify that this math is correct. i think i'm orphaning some memory
		bytes_per_core.push_back( (UINT64) (TOTAL_DRAM_MEMORY_BYTES / num_nodes) );
		ADDRINT start_addr = bytes_per_core[i] * i;
		ADDRINT end_addr = (ADDRINT) bytes_per_core[i] * (i + 1) - 1;
		address_boundaries.push_back( pair<ADDRINT, ADDRINT>(start_addr, end_addr) );
#ifdef AHL_DEBUG
		cout << " Start Addr: " << hex << start_addr << " End_Addr: " << hex << end_addr << endl;
#endif
	}
}

AddressHomeLookup::AddressHomeLookup(vector< pair<ADDRINT,ADDRINT> > addr_bounds, INT32 ahl_id_arg)
{
	ahl_id = ahl_id_arg;
	address_boundaries = addr_bounds;
	num_nodes = address_boundaries.size();
}

AddressHomeLookup::~AddressHomeLookup()
{
	//TODO deallocate vectors?
//	~address_boundaries;
}


void AddressHomeLookup::setAddrBoundaries(vector< pair<ADDRINT, ADDRINT> > addr_bounds) 
{
	//FIXME memory issue. is this correct? do i need to deallocate old addr_boundaries?
	//copymethod? etc. etc.
	address_boundaries = addr_bounds;
}

*/

/*
 * Given an address, returns the home node number where the address's dram directory lives
 */

/*

UINT32 AddressHomeLookup::find_home_for_addr(ADDRINT address) const {

	INT32 return_core_id = -1;
	UINT32 index = 0;


	
	
	//TODO OPTIMIZE? can we make address home lookup faster than this, but still flexible?
	while(return_core_id == -1 && index < num_nodes) {
		
		if( (ADDRINT) address >= address_boundaries[index].first
			&& (ADDRINT) address < address_boundaries[index].second ) 
		{
#ifdef AHL_DEBUG
			cout << "ADDR: " << hex << address << " Index: " << dec << index << ", NumCores: " << num_nodes << endl;
#endif
			return_core_id = index;
		}
	
	   ++index;
	}
	
#ifdef AHL_DEBUG
	stringstream ss;
	ss << "ADDR: " << hex << address << "  HOME_CORE: " << dec << return_core_id;
	debugPrint(ahl_id, "AHL", ss.str());
#endif
	
	assert( return_core_id < (INT32) num_nodes);
	//hard to say what behavior should be for addresses "out of bounds"
	//perhaps only set up shared memory, and anything not "in bounds"
	//would be considered private memory?  Or just automatically home it
	//on core 0?
	return_core_id = ( return_core_id == -1 ) ? 0 : return_core_id;
	assert( return_core_id >= 0 );
	
	//a bit of incompatibilities of int32 vs uint32
	return (UINT32) return_core_id;

//commented out old static style of even distribution
//  int dram_line = address / bytes_per_cache_line;
//  UINT32 return_core_id = dram_line / cache_lines_per_node;
//  return return_core_id;
//  

}

*/

