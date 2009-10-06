#ifndef __DIRECTORY_ENTRY_LIMITED_BROADCAST_H__
#define __DIRECTORY_ENTRY_LIMITED_BROADCAST_H__

#include "directory_entry.h"

class DirectoryEntryLimitedBroadcast : public DirectoryEntry
{
   private:
      bool m_global_enabled;
      UInt32 m_number_of_sharers;

   public:

      DirectoryEntryLimitedBroadcast(UInt32 max_hw_sharers, UInt32 max_num_sharers);
      ~DirectoryEntryLimitedBroadcast();
      
      bool hasSharer(SInt32 sharer_id);
      std::pair<bool, SInt32> addSharer(SInt32 sharer_id);
      void removeSharer(SInt32 sharer_id);
      UInt32 numSharers();

      SInt32 getOwner();
      void setOwner(SInt32 owner_id);

      std::pair<bool, std::vector<SInt32> > getSharersList();

      UInt32 getLatency();
};

#endif /* __DIRECTORY_ENTRY_LIMITED_BROADCAST_H__ */
