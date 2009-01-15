#include "address_home_lookup.h"

AddressHomeLookup::AddressHomeLookup (UInt32 num_nodes_arg, UInt32 log_block_size_arg, SInt32 ahl_id_arg) {

	// Each Block Address is as follows:
	// /////////////////////////////////////////////////////////// //
	//   block_num               |   block_offset                  //
	// /////////////////////////////////////////////////////////// //
		
	ahl_id = ahl_id_arg;
	num_nodes = num_nodes_arg;
	log_block_size = log_block_size_arg;

}

UInt32 AddressHomeLookup::find_home_for_addr(IntPtr address) const {

	UInt32 node = (address >> log_block_size) % num_nodes;
	assert (0 <= node && node < num_nodes);
	return (node);

}

AddressHomeLookup::~AddressHomeLookup() {
	// TODO: Some meaningful scheme
}

#if 0 
int main (int argc, char *argv[]) {

	AddressHomeLookup addrLookupTable(13, 5, 0);

	assert (addrLookupTable.find_home_for_addr((152 << 5) | 31) == 9);
	assert (addrLookupTable.find_home_for_addr((0 << 5) | 0) == 0);
	assert (addrLookupTable.find_home_for_addr((169 << 5) | 31) == 0);
	assert (addrLookupTable.find_home_for_addr((168 << 5) | 31) == 12);
	assert (addrLookupTable.find_home_for_addr(12345) == 8);

	cout << "All Tests Passed\n";
}
#endif
	
