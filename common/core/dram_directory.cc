#include "dram_directory.h"
#include "debug.h"

DramDirectory::DramDirectory(UINT32 num_lines_arg, UINT32 bytes_per_cache_line_arg, UINT32 dram_id_arg, UINT32 num_of_cores_arg)
{
	num_lines = num_lines_arg;
	number_of_cores = num_of_cores_arg;
	dram_id = dram_id_arg;
  cout << "Init Dram with num_lines: " << num_lines << endl;
  cout << "   bytes_per_cache_linew: " << bytes_per_cache_line_arg << endl;
  assert( num_lines >= 0 );
//  dram_directory_entries = new DramDirectoryEntry[num_lines];
//  for(UINT32 i =0; i < num_lines; i++) {
	  //FIXME is this correct or is there a better way of initializing dramEntry?
//	  dram_directory_entries[i].dramDirEntryInit(number_of_cores);
//  }
  bytes_per_cache_line = bytes_per_cache_line_arg;
  
  // note: all dram_directory_entries are initialized properly by the dram_directory_entry constructor
}

DramDirectory::~DramDirectory()
{
  //FIXME is this correct way to delete a std::map? and is std::map going to deallocate all of the entries?
//  delete dram_directory_entries;
}

/*
 * returns the associated DRAM directory entry given a memory address
 */
DramDirectoryEntry* DramDirectory::getEntry(ADDRINT address)
{

	// note: the directory is a map key'ed by cache line. so, first we need to determine the associated cache line
	UINT32 cache_line_index = (address / bytes_per_cache_line) - ( num_lines * dram_id );
  
#ifdef DRAM_DEBUG
	printf(" DRAM_DIR: getEntry: address        = 0x %x\n" ,address );
	cout << " DRAM_DIR: getEntry: bytes_per_$line= " << bytes_per_cache_line << endl;
	cout << " DRAM_DIR: getEntry: cachline_index = " << cache_line_index << endl;
	cout << " DRAM_DIR: getEntry: number of lines= " << num_lines << endl;
#endif
  
	assert( cache_line_index < num_lines);
	assert( cache_line_index >= 0);

	DramDirectoryEntry* entry_ptr = dram_directory_entries[cache_line_index];
  
	if( entry_ptr == NULL ) {
		dram_directory_entries[cache_line_index] =  new DramDirectoryEntry(cache_line_index, number_of_cores);
	}

	return dram_directory_entries[cache_line_index];
}
