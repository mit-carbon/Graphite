#ifndef CACHE_DIRECTORY_H
#define CACHE_DIRECTORY_H

#include "assert.h"
#include "pin.H"
#include "cache_directory_entry.h"

class CacheDirectory
{
private:
	UINT32 num_lines;
	unsigned int bytes_per_cache_line;
	CacheDirectoryEntry *cache_directory_entries;
	
public:
	CacheDirectory(UINT32 num_lines, unsigned int bytes_per_cache_line);
	virtual ~CacheDirectory();
	
	CacheDirectoryEntry getEntry(ADDRINT index);
	
};

#endif
