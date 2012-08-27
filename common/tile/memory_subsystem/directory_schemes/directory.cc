#include "simulator.h"
#include "directory.h"
#include "directory_entry.h"
#include "log.h"

Directory::Directory(CachingProtocolType caching_protocol_type, DirectoryType directory_type,
                     SInt32 total_entries, SInt32 max_hw_sharers, SInt32 max_num_sharers)
   : _total_entries(total_entries)
{
   // Look at the type of directory and create 
   _directory_entry_list.resize(_total_entries);
  
   for (SInt32 i = 0; i < _total_entries; i++)
   {
      _directory_entry_list[i] = DirectoryEntry::create(caching_protocol_type, directory_type,
                                                        max_hw_sharers, max_num_sharers);
   }

   // Sharer Stats
   initializeSharerStats();
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
   _sharer_count_vec.resize(Config::getSingleton()->getTotalTiles()+1, 0);
}

void
Directory::updateSharerStats(SInt32 old_sharer_count, SInt32 new_sharer_count)
{
   assert(old_sharer_count >= 0 && old_sharer_count < (SInt32) Config::getSingleton()->getTotalTiles());
   assert(new_sharer_count >= 0 && old_sharer_count < (SInt32) Config::getSingleton()->getTotalTiles());
   if (old_sharer_count > 0)
   {
      assert(_sharer_count_vec[old_sharer_count] > 0);
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
   sharer_count_vec = _sharer_count_vec;
}
