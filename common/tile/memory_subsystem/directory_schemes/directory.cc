#include "simulator.h"
#include "directory.h"
#include "directory_entry.h"
#include "log.h"

Directory::Directory(CachingProtocolType caching_protocol_type, DirectoryType directory_type,
                     SInt32 total_entries, SInt32 max_hw_sharers, SInt32 max_num_sharers)
   : _total_entries(total_entries)
   , _directory_type(directory_type)
{
   // Look at the type of directory and create 
   _directory_entry_list.resize(_total_entries);
  
   for (SInt32 i = 0; i < _total_entries; i++)
   {
      _directory_entry_list[i] = DirectoryEntry::create(caching_protocol_type, _directory_type,
                                                        max_hw_sharers, max_num_sharers);
   }

   if (_directory_type == FULL_MAP)
   {
      // Sharer Stats
      initializeSharerStats();
   }
}

Directory::~Directory()
{
   for (SInt32 i = 0; i < _total_entries; i++)
   {
      delete _directory_entry_list[i];
   }
}

DirectoryEntry*
Directory::getDirectoryEntry(SInt32 entry_num)
{
   return _directory_entry_list[entry_num]; 
}

void
Directory::setDirectoryEntry(SInt32 entry_num, DirectoryEntry* directory_entry)
{
   _directory_entry_list[entry_num] = directory_entry;
}

void
Directory::initializeSharerStats()
{
   LOG_ASSERT_ERROR(_directory_type == FULL_MAP, "Directory type should be FULL_MAP, now (%i)", _directory_type);
   _sharer_count_vec.resize(Config::getSingleton()->getTotalTiles()+1, 0);
}

void
Directory::updateSharerStats(SInt32 old_sharer_count, SInt32 new_sharer_count)
{
   LOG_ASSERT_ERROR(_directory_type == FULL_MAP, "Directory type should be FULL_MAP, now (%i)", _directory_type);

   LOG_ASSERT_ERROR(old_sharer_count >= 0 && old_sharer_count <= (SInt32) Config::getSingleton()->getTotalTiles(),
         "old_sharer_count must be within [0,%u], now (%i)", Config::getSingleton()->getTotalTiles(), old_sharer_count);
   LOG_ASSERT_ERROR(new_sharer_count >= 0 && new_sharer_count <= (SInt32) Config::getSingleton()->getTotalTiles(),
         "new_sharer_count must be within [0,%u], now (%i)", Config::getSingleton()->getTotalTiles(), new_sharer_count);
   if (old_sharer_count > 0)
   {
      LOG_ASSERT_ERROR(_sharer_count_vec[old_sharer_count] > 0, "_sharer_count_vec[%i] must be > 0, now (%i)",
            old_sharer_count, _sharer_count_vec[old_sharer_count]);
      _sharer_count_vec[old_sharer_count] --;
   }
   if (new_sharer_count > 0)
   {
      _sharer_count_vec[new_sharer_count] ++;
   }
}

void
Directory::getSharerStats(vector<UInt64>& sharer_count_vec)
{
   LOG_ASSERT_ERROR(_directory_type == FULL_MAP, "Directory type should be FULL_MAP, now (%i)", _directory_type);
   sharer_count_vec = _sharer_count_vec;
}
