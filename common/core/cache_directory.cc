#include "cache_directory.h"

CacheDirectory::CacheDirectory(int num_lines, unsigned int bytes_per_cache_line_arg)
{
  cache_directory_entries = new CacheDirectoryEntry[num_lines];
  bytes_per_cache_line = bytes_per_cache_line_arg;
  
  // note: all cache_directory_entries are initialized properly by the cache_directory_entry constructor
}

CacheDirectory::~CacheDirectory()
{
  delete[] cache_directory_entries;
}

CacheDirectoryEntry CacheDirectory::getEntry(int address)
{
  // note: the directory is an array indexed by cache line. so, first we need to determine the associated cache line
  int cache_line_index = address / bytes_per_cache_line;
  return cache_directory_entries[cache_line_index];
}
