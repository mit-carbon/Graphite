#include "directory_type.h"
#include "directory_entry.h"
#include "directory_entry_full_map.h"
#include "directory_entry_limited_broadcast.h"
#include "directory_entry_limited_no_broadcast.h"
#include "directory_entry_ackwise.h"
#include "directory_entry_limitless.h"
#include "utils.h"
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

DirectoryType
DirectoryEntry::parseDirectoryType(string directory_type)
{
   if (directory_type == "full_map")
      return FULL_MAP;
   else if (directory_type == "limited_no_broadcast")
      return LIMITED_NO_BROADCAST;
   else if (directory_type == "limited_broadcast")
      return LIMITED_BROADCAST;
   else if (directory_type == "ackwise")
      return ACKWISE;
   else if (directory_type == "limitless")
      return LIMITLESS;
   else
      LOG_PRINT_ERROR("Unsupported Directory Type: %s", directory_type.c_str());
   return (DirectoryType) -1;
}

DirectoryEntry*
DirectoryEntry::create(CachingProtocolType caching_protocol_type, DirectoryType directory_type, SInt32 max_hw_sharers, SInt32 max_num_sharers)
{
   return create(directory_type, max_hw_sharers, max_num_sharers);
}

DirectoryEntry*
DirectoryEntry::create(DirectoryType directory_type, SInt32 max_hw_sharers, SInt32 max_num_sharers)
{
   switch (directory_type)
   {
   case FULL_MAP:
      return new DirectoryEntryFullMap(max_num_sharers);

   case LIMITED_NO_BROADCAST:
      return new DirectoryEntryLimitedNoBroadcast(max_hw_sharers);

   case LIMITED_BROADCAST:
      return new DirectoryEntryLimitedBroadcast(max_hw_sharers);

   case ACKWISE:
      return new DirectoryEntryAckwise(max_hw_sharers);

   case LIMITLESS:
      return new DirectoryEntryLimitless(max_hw_sharers, max_num_sharers);

   default:
      LOG_PRINT_ERROR("Unrecognized Directory Type: %u", directory_type);
      return NULL;
   }
}

UInt32
DirectoryEntry::getSize(DirectoryType directory_type, SInt32 max_hw_sharers, SInt32 max_num_sharers)
{
   LOG_PRINT("DirectoryEntry::getSize(), Directory Type(%u), Max Num Sharers(%i), Max HW Sharers(%i)", directory_type, max_num_sharers, max_hw_sharers);
   switch(directory_type)
   {
   case FULL_MAP:
      return max_num_sharers;
   case LIMITED_NO_BROADCAST:
   case LIMITED_BROADCAST:
   case ACKWISE:
   case LIMITLESS:
      return max_hw_sharers * ceilLog2(max_num_sharers);
   default:
      LOG_PRINT_ERROR("Unrecognized directory type(%u)", directory_type);
      return 0;
   }
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
