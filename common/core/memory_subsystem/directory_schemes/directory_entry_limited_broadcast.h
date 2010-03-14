#ifndef __DIRECTORY_ENTRY_LIMITED_BROADCAST_H__
#define __DIRECTORY_ENTRY_LIMITED_BROADCAST_H__

#include "directory_entry.h"

class DirectoryEntryLimitedBroadcast : public DirectoryEntry
{
   private:
      bool m_global_enabled;
      UInt32 m_num_untracked_sharers;

   public:

      DirectoryEntryLimitedBroadcast(UInt32 max_hw_sharers, UInt32 max_num_sharers);
      ~DirectoryEntryLimitedBroadcast();
      
      bool hasSharer(core_id_t sharer_id);
      bool addSharer(core_id_t sharer_id);
      void removeSharer(core_id_t sharer_id);
      UInt32 getNumSharers();

      core_id_t getOwner();
      void setOwner(core_id_t owner_id);

      core_id_t getOneSharer();
      std::pair<bool, std::vector<core_id_t> >& getSharersList();

      UInt32 getLatency();
};

#endif /* __DIRECTORY_ENTRY_LIMITED_BROADCAST_H__ */
