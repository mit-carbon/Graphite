#ifndef CACHE_DIRECTORY_H
#define CACHE_DIRECTORY_H

#include "cache_directory_entry.h"

class CacheDirectory
{
private:
	int num_lines;
	CacheDirectoryEntry *cache_directory_entries;
	
public:
	CacheDirectory(int num_lines);
	virtual ~CacheDirectory();
	
	CacheDirectoryEntry getEntry(int index);
	
};

#endif
