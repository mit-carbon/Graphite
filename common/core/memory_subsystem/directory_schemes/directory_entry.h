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
      tile_id_t m_owner_id;
      IntPtr m_address;
      BitVector* m_sharers;
      DirectoryBlockInfo* m_directory_block_info;

      std::pair<bool, std::vector<tile_id_t> > m_cached_sharers_list;

   public:
      DirectoryEntry(UInt32 max_hw_sharers, UInt32 max_num_sharers);
      virtual ~DirectoryEntry();

      DirectoryBlockInfo* getDirectoryBlockInfo();

      virtual bool hasSharer(tile_id_t sharer_id) = 0;
      virtual bool addSharer(tile_id_t sharer_id) = 0;
      virtual void removeSharer(tile_id_t sharer_id, bool reply_expected = false) = 0;
      virtual UInt32 getNumSharers() = 0;

      virtual tile_id_t getOwner() = 0;
      virtual void setOwner(tile_id_t owner_id) = 0;

      IntPtr getAddress() { return m_address; }
      void setAddress(IntPtr address) { m_address = address; }

      virtual tile_id_t getOneSharer() = 0;
      virtual std::pair<bool, std::vector<tile_id_t> >& getSharersList() = 0;

      virtual UInt32 getLatency() = 0;
};

#endif /* __DIRECTORY_ENTRY_H__ */
