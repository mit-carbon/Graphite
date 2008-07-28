#include "cache_directory.h"

CacheDirectory::CacheDirectory(int num_lines)
{
   cache_directory_entries = new CacheDirectoryEntry[num_lines];
}

CacheDirectory::~CacheDirectory()
{
   delete[] cache_directory_entries;
}

CacheDirectoryEntry& CacheDirectory::getEntry(int address)
{
	// note: the directory is an array indexed by cache line. so, first we need to determine the associated cache line
	int cache_line_index = address / ocache->getLineSize();  // TODO: expose visibility to ocache
	return cache_directory_entries[cache_line_index];
}
