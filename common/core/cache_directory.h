#ifndef CACHE_DIRECTORY_H
#define CACHE_DIRECTORY_H

#include "cache_directory_entry.h"

class CacheDirectory
{
private:
	int num_lines;
	unsigned int bytes_per_cache_line;
	CacheDirectoryEntry *cache_directory_entries;
	
public:
	CacheDirectory(int num_lines, unsigned int bytes_per_cache_line);
	virtual ~CacheDirectory();
	
	CacheDirectoryEntry getEntry(int index);
	
};

#endif
