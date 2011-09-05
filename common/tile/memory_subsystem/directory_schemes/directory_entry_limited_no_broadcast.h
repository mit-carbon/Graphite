#pragma once

#include "directory_entry_limited.h"

class DirectoryEntryLimitedNoBroadcast : public DirectoryEntryLimited
{
public:
   DirectoryEntryLimitedNoBroadcast(SInt32 max_hw_sharers);
   ~DirectoryEntryLimitedNoBroadcast();
   
   void removeSharer(tile_id_t sharer_id, bool reply_expected);

   UInt32 getLatency();
};
