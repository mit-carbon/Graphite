#ifndef __DIRECTORY_ENTRY_LIMITLESS_H__
#define __DIRECTORY_ENTRY_LIMITLESS_H__

#include "directory_entry.h"

class DirectoryEntryLimitless : public DirectoryEntry
{
   private:
      bool m_software_trap_enabled;
      UInt32 m_software_trap_penalty;

   public:
      DirectoryEntryLimitless(UInt32 max_hw_sharers, 
            UInt32 max_num_sharers, 
            UInt32 software_trap_penalty);
      ~DirectoryEntryLimitless();
      
      bool hasSharer(core_id_t sharer_id);
      bool addSharer(core_id_t sharer_id);
      void removeSharer(core_id_t sharer_id, bool reply_expected);
      UInt32 getNumSharers();

      core_id_t getOwner();
      void setOwner(core_id_t owner_id);

      core_id_t getOneSharer();
      std::pair<bool, std::vector<core_id_t> >& getSharersList();

      UInt32 getLatency();
};

#endif /* __DIRECTORY_ENTRY_LIMITLESS_H__ */
