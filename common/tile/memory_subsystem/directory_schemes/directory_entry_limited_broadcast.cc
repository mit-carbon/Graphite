#include "directory_entry_limited_broadcast.h"
#include "config.h"
#include "log.h"

using namespace std;

DirectoryEntryLimitedBroadcast::DirectoryEntryLimitedBroadcast(SInt32 max_hw_sharers)
   : DirectoryEntryLimited(max_hw_sharers)
   , _global_enabled(false)
   , _num_sharers(0)
{}

DirectoryEntryLimitedBroadcast::~DirectoryEntryLimitedBroadcast()
{}

// Returns: Says whether the sharer was successfully added
//              'True' if it was successfully added
//              'False' if there will be an eviction before adding
bool
DirectoryEntryLimitedBroadcast::addSharer(tile_id_t sharer_id)
{
   if (!_global_enabled)   // Not all sharers are explicitly tracked
   {
      bool added = DirectoryEntryLimited::addSharer(sharer_id);
      if (!added) // Reached the limit on hardware sharers
      {
         LOG_ASSERT_ERROR((_num_sharers == 0) && (!_global_enabled) && (_num_tracked_sharers == _max_hw_sharers),
                          "Num Sharers(%i), Global Enabled(%s), Num Tracked Sharers(%i), Max HW Sharers(%i)",
                          _num_sharers, _global_enabled ? "true" : "false", _num_tracked_sharers, _max_hw_sharers);
         _global_enabled = true;
         _num_sharers = Config::getSingleton()->getTotalTiles();
      }
   }
   return true;
}

void
DirectoryEntryLimitedBroadcast::removeSharer(tile_id_t sharer_id, bool reply_expected)
{
   if (_global_enabled) // Not all sharers are explicitly tracked
   {
      if (DirectoryEntryLimited::hasSharer(sharer_id))
         DirectoryEntryLimited::removeSharer(sharer_id);
   }
   else // (All sharers are explicitly tracked)
   {
      DirectoryEntryLimited::removeSharer(sharer_id);
   }

   if (reply_expected) // Reply to an invalidate request
   {
      LOG_ASSERT_ERROR(_global_enabled, "Global Enabled(false)");
      _num_sharers --;
      if (_num_sharers == 0)
      {
         LOG_ASSERT_ERROR(_num_tracked_sharers == 0, "Num Tracked Sharers(%i)", _num_tracked_sharers);
         _global_enabled = false;
      }
   }
}

SInt32
DirectoryEntryLimitedBroadcast::getNumSharers()
{
   return _global_enabled ? _num_sharers : _num_tracked_sharers;
}

bool
DirectoryEntryLimitedBroadcast::inBroadcastMode()
{
   return _global_enabled;
}

// Return a pair:
// val.first :- 'True' if all tiles are sharers
//              'False' if NOT all tiles are sharers
// val.second :- A list of tracked sharers
bool
DirectoryEntryLimitedBroadcast::getSharersList(vector<tile_id_t>& sharers_list)
{
   DirectoryEntryLimited::getSharersList(sharers_list);
   return _global_enabled;
}

UInt32
DirectoryEntryLimitedBroadcast::getLatency()
{
   return 0;
}

