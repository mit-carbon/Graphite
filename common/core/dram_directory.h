#ifndef DRAM_DIRECTORY_H
#define DRAM_DIRECTORY_H

#include "dram_directory_entry.h"

class DramDirectory
{
private:
	int num_lines;
	DramDirectoryEntry *dram_directory_entries;
	
public:
	DramDirectory(int num_lines);
	virtual ~DramDirectory();
	
};

#endif
