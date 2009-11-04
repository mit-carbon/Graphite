#ifndef __DIRECTORY_ENTRY_H__
#define __DIRECTORY_ENTRY_H__

#include <vector>

#include "bit_vector.h"
#include "directory_block_info.h"

class DirectoryEntry
{
   protected:
      UInt32 m_max_hw_sharers;
      UInt32 m_max_num_sharers;
      SInt32 m_owner_id;
      IntPtr m_address;
      BitVector* m_sharers;
      DirectoryBlockInfo* m_directory_block_info;

   public:
      DirectoryEntry(UInt32 max_hw_sharers, UInt32 max_num_sharers);
      virtual ~DirectoryEntry();

      DirectoryBlockInfo* getDirectoryBlockInfo();

      virtual bool hasSharer(SInt32 sharer_id) = 0;
      virtual std::pair<bool, SInt32> addSharer(SInt32 sharer_id) = 0;
      virtual void removeSharer(SInt32 sharer_id) = 0;
      virtual UInt32 numSharers() = 0;

      virtual SInt32 getOwner() = 0;
      virtual void setOwner(SInt32 owner_id) = 0;

      IntPtr getAddress() { return m_address; }
      void setAddress(IntPtr address) { m_address = address; }

      virtual std::pair<bool, std::vector<SInt32> > getSharersList() = 0;

      virtual UInt32 getLatency() = 0;
};

#endif /* __DIRECTORY_ENTRY_H__ */
