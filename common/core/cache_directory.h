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
	
	//debugging
	UINT32 cache_id;
	
public:
	CacheDirectory(UINT32 num_lines, unsigned int bytes_per_cache_line, UINT32 cache_id);
	virtual ~CacheDirectory();
	
	CacheDirectoryEntry* getEntry(ADDRINT index);
	void print();
	
};

#endif
