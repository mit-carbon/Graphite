#include "dram_directory.h"

DramDirectory::DramDirectory(int num_lines)
{
   dram_directory_entries = new DramDirectoryEntry[num_lines];
}

DramDirectory::~DramDirectory()
{
   delete[] dram_directory_entries;
}


/*
 * returns the associated DRAM directory entry given a memory address
 */

DramDirectoryEntry DramDirectory::getEntry(int address)
{
	// note: the directory is an array indexed by cache line. so, first we need to determine the associated cache line
	int cache_line_index = address / ocache->getLineSize();  // TODO: expose visibility to ocache
	return dram_directory_entries[cache_line_index];
}
