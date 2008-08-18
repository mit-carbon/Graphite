#include "dram_directory.h"

DramDirectory::DramDirectory(UINT32 num_lines_arg, unsigned int bytes_per_cache_line_arg, UINT32 dram_id_arg)
{
	num_lines = num_lines_arg;
	dram_id = dram_id_arg;
  cout << "Init Dram with num_lines: " << num_lines << endl;
  cout << "   bytes_per_cache_linew: " << bytes_per_cache_line_arg << endl;
  assert( num_lines >= 0 );
  dram_directory_entries = new DramDirectoryEntry[num_lines];
  bytes_per_cache_line = bytes_per_cache_line_arg;
  
  // note: all dram_directory_entries are initialized properly by the dram_directory_entry constructor
}

DramDirectory::~DramDirectory()
{
  delete[] dram_directory_entries;
}


/*
 * returns the associated DRAM directory entry given a memory address
 */
DramDirectoryEntry DramDirectory::getEntry(ADDRINT address)
{
  // note: the directory is an array indexed by cache line. so, first we need to determine the associated cache line
  
  
	cout << " DRAM_DIR: getEntry: address        = " << address << endl;

  UINT32 cache_line_index = (address / bytes_per_cache_line) - ( num_lines * dram_id );
  cout << " DRAM_DIR: getEntry: bytes_per_$line= " << bytes_per_cache_line << endl;
  cout << " DRAM_DIR: getEntry: cachline_index = " << cache_line_index << endl;
  cout << " DRAM_DIR: getEntry: number of lines= " << num_lines << endl;
  assert( cache_line_index < num_lines);
  assert( cache_line_index >= 0);
  return dram_directory_entries[cache_line_index];
}
