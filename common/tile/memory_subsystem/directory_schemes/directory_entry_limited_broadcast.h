#pragma once

#include "directory_entry_limited.h"

class DirectoryEntryLimitedBroadcast : public DirectoryEntryLimited
{
public:
   DirectoryEntryLimitedBroadcast(SInt32 max_hw_sharers);
   ~DirectoryEntryLimitedBroadcast();
   
   bool addSharer(tile_id_t sharer_id);
   void removeSharer(tile_id_t sharer_id, bool reply_expected);
  
   bool inBroadcastMode(); 
   bool getSharersList(vector<tile_id_t>& sharers_list);
   SInt32 getNumSharers();

   UInt32 getLatency();

private:
   bool _global_enabled;
   UInt32 _num_sharers;
};
