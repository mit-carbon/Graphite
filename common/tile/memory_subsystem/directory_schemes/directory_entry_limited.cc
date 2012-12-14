#include "directory_entry_limited.h"
#include "log.h"

using namespace std;

DirectoryEntryLimited::DirectoryEntryLimited(SInt32 max_hw_sharers)
   : DirectoryEntry(max_hw_sharers)
   , _num_tracked_sharers(0)
{
   _sharers.resize(_max_hw_sharers);
   for (SInt32 i = 0; i < _max_hw_sharers; i++)
      _sharers[i] = INVALID_SHARER;
}

DirectoryEntryLimited::~DirectoryEntryLimited()
{}

bool
DirectoryEntryLimited::hasSharer(tile_id_t sharer_id)
{
   for (SInt32 i = 0; i < _max_hw_sharers; i++)
   {
      if (_sharers[i] == sharer_id)
         return true;
   }
   return false;
}

// Returns: Says whether the sharer was successfully added
//              'True' if it was successfully added
//              'False' if there will be an eviction before adding
bool
DirectoryEntryLimited::addSharer(tile_id_t sharer_id)
{
   for (SInt32 i = 0; i < _max_hw_sharers; i++)
   {
      LOG_ASSERT_ERROR(_sharers[i] != (SInt16) sharer_id, "Sharer(%i) already present", sharer_id);
   }
   // Be sure that the sharer is not already added
   for (SInt32 i = 0; i < _max_hw_sharers; i++)
   {
      if (_sharers[i] == INVALID_SHARER)
      {
         _sharers[i] = (SInt16) sharer_id;
         _num_tracked_sharers ++;
         return true;
      }
   }
   return false;
}

void
DirectoryEntryLimited::removeSharer(tile_id_t sharer_id)
{
   for (SInt32 i = 0; i < _max_hw_sharers; i++)
   {
      if (_sharers[i] == (SInt16) sharer_id)
      {
         _sharers[i] = INVALID_SHARER;
         _num_tracked_sharers --;
         return;
      }
   }
   LOG_PRINT_ERROR("Could not find sharer_id(%i)", sharer_id);
}

// One sharer from the list of tracked sharers
tile_id_t
DirectoryEntryLimited::getOneSharer()
{
   vector<tile_id_t> sharers_list;
   getSharersList(sharers_list);
   
   if (sharers_list.empty())
   {
      return INVALID_TILE_ID;
   }
   else
   {
      SInt32 index = _rand_num.next(sharers_list.size());
      return sharers_list[index];
   }
}

// A list of tracked sharers
bool
DirectoryEntryLimited::getSharersList(vector<tile_id_t>& sharers_list)
{
   for (SInt32 i = 0; i < _max_hw_sharers; i++)
   {
      if (_sharers[i] != INVALID_SHARER)
         sharers_list.push_back((tile_id_t) _sharers[i]);
   }
   LOG_ASSERT_ERROR(_num_tracked_sharers == (SInt32) sharers_list.size(),
                    "Num Tracked Sharers(%i), Sharers List Size(%u)",
                    _num_tracked_sharers, sharers_list.size());
   return true;
}

// Number of Tracked Sharers
SInt32
DirectoryEntryLimited::getNumSharers()
{
   return _num_tracked_sharers;
}
