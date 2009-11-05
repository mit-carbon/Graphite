#include "directory_entry.h"

DirectoryEntry::DirectoryEntry(UInt32 max_hw_sharers, UInt32 max_num_sharers):
   m_max_hw_sharers(max_hw_sharers),
   m_max_num_sharers(max_num_sharers),
   m_owner_id(INVALID_CORE_ID),
   m_address(INVALID_ADDRESS)
{
   m_sharers = new BitVector(m_max_num_sharers);
   m_directory_block_info = new DirectoryBlockInfo();
}

DirectoryEntry::~DirectoryEntry()
{
   delete m_directory_block_info;
   delete m_sharers;
}

DirectoryBlockInfo*
DirectoryEntry::getDirectoryBlockInfo()
{
   return m_directory_block_info;
}
