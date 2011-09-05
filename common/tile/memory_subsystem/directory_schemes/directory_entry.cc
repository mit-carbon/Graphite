#include "directory_entry.h"
#include "log.h"

DirectoryEntry::DirectoryEntry(SInt32 max_hw_sharers)
   : _address(INVALID_ADDRESS)
   , _owner_id(INVALID_TILE_ID)
   , _max_hw_sharers(max_hw_sharers)
{
   _directory_block_info = new DirectoryBlockInfo();
}

DirectoryEntry::~DirectoryEntry()
{
   delete _directory_block_info;
}

DirectoryBlockInfo*
DirectoryEntry::getDirectoryBlockInfo()
{
   return _directory_block_info;
}
   
tile_id_t
DirectoryEntry::getOwner()
{
   return _owner_id;
}

void
DirectoryEntry::setOwner(tile_id_t owner_id)
{
   if (owner_id != INVALID_TILE_ID)
      LOG_ASSERT_ERROR(hasSharer(owner_id), "Owner Id(%i) not a sharer", owner_id);
   _owner_id = owner_id;
}
