#ifndef __DIRECTORY_ENTRY_LIMITED_NO_BROADCAST_H__
#define __DIRECTORY_ENTRY_LIMITED_NO_BROADCAST_H__

#include "directory_entry.h"

class DirectoryEntryLimitedNoBroadcast : public DirectoryEntry
{
   private:
      SInt32 getNextEvictionId();

   public:
      DirectoryEntryLimitedNoBroadcast(UInt32 max_hw_sharers, UInt32 max_num_sharers);
      ~DirectoryEntryLimitedNoBroadcast();
      
      bool hasSharer(SInt32 sharer_id);
      std::pair<bool, SInt32> addSharer(SInt32 sharer_id);
      void removeSharer(SInt32 sharer_id);
      UInt32 numSharers();

      SInt32 getOwner();
      void setOwner(SInt32 owner_id);

      std::pair<bool, std::vector<SInt32> > getSharersList();

      UInt32 getLatency();
};

#endif /* __DIRECTORY_ENTRY_LIMITED_NO_BROADCAST_H__ */
