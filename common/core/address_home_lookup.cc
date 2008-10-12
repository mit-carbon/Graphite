#include "address_home_lookup.h"
//move this back into the makefile system...
#define TOTAL_DRAM_MEMORY_BYTES (pow(2,32))
//#define AHL_DEBUG

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
	//poorly assumes that each node is given contingious section of memory
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
	cerr << "Inside AHL " << endl;
	address_boundaries = addr_bounds;
	cerr << "Finished AHL " << endl;
}

/*
 * Given an address, returns the home node number where the address's dram directory lives
 */
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
}
