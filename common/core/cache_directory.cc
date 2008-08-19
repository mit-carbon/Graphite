#include "cache_directory.h"

CacheDirectory::CacheDirectory(UINT32 num_lines_args, unsigned int bytes_per_cache_line_arg)
{
  num_lines = num_lines_args;
  cache_directory_entries = new CacheDirectoryEntry[num_lines];
  bytes_per_cache_line = bytes_per_cache_line_arg;
  
  // note: all cache_directory_entries are initialized properly by the cache_directory_entry constructor
}

CacheDirectory::~CacheDirectory()
{
  delete[] cache_directory_entries;
}

CacheDirectoryEntry CacheDirectory::getEntry(ADDRINT address)
{
  // note: the directory is an array indexed by cache line. so, first we need to determine the associated cache line
  assert( address > 0 );
  UINT32 cache_line_index = (address / bytes_per_cache_line) % num_lines;

#ifdef CDIR_DEBUG
  cout << "   CacheDirtory: getEntry for Addr : " << address << endl;
  cout << "   CacheDirtory: Addr / bytes per $L: " << address / (bytes_per_cache_line) << endl;
  cout << "   CacheDirtory: getEntry - cache_line_index: " << cache_line_index << endl;
  cout << "   CacheDirtory: getEntry - num_lines: " << num_lines << endl;
  assert( cache_line_index < num_lines );
#endif

//  int cache_line_index = address / bytes_per_cache_line;
  return cache_directory_entries[cache_line_index];
}
