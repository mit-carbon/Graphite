using namespace std;

#include "simulator.h"
#include "directory.h"
#include "directory_entry.h"
#include "directory_entry_full_map.h"
#include "directory_entry_limited_broadcast.h"
#include "directory_entry_limited_no_broadcast.h"
#include "directory_entry_ackwise.h"
#include "directory_entry_limitless.h"
#include "log.h"
#include "utils.h"

Directory::Directory(string directory_type_str, SInt32 num_entries, SInt32 max_hw_sharers, SInt32 max_num_sharers)
   : _num_entries(num_entries)
   , _max_hw_sharers(max_hw_sharers)
   , _max_num_sharers(max_num_sharers)
{
   // Look at the type of directory and create 
   _directory_entry_list.resize(_num_entries);
  
   _directory_type = parseDirectoryType(directory_type_str);
   for (SInt32 i = 0; i < _num_entries; i++)
   {
      _directory_entry_list[i] = createDirectoryEntry();
   }
}

Directory::~Directory()
{
   for (SInt32 i = 0; i < _num_entries; i++)
   {
      delete _directory_entry_list[i];
   }
}

DirectoryEntry*
Directory::getDirectoryEntry(SInt32 entry_num)
{
   return _directory_entry_list[entry_num]; 
}

UInt32
Directory::getDirectoryEntrySize()
{
   LOG_PRINT("getDirectoryEntrySize(%u)", _directory_type);
   switch(_directory_type)
   {
   case FULL_MAP:
      return _max_num_sharers;
   case LIMITED_NO_BROADCAST:
   case LIMITED_BROADCAST:
   case ACKWISE:
   case LIMITLESS:
      return _max_hw_sharers * ceilLog2(_max_num_sharers);
   default:
      LOG_PRINT_ERROR("Unrecognized directory type(%u)", _directory_type);
      return 0;
   }
}

void
Directory::setDirectoryEntry(SInt32 entry_num, DirectoryEntry* directory_entry)
{
   _directory_entry_list[entry_num] = directory_entry;
}

Directory::DirectoryType
Directory::parseDirectoryType(string directory_type_str)
{
   if (directory_type_str == "full_map")
      return FULL_MAP;
   else if (directory_type_str == "limited_no_broadcast")
      return LIMITED_NO_BROADCAST;
   else if (directory_type_str == "limited_broadcast")
      return LIMITED_BROADCAST;
   else if (directory_type_str == "ackwise")
      return ACKWISE;
   else if (directory_type_str == "limitless")
      return LIMITLESS;
   else
   {
      LOG_PRINT_ERROR("Unsupported Directory Type: %s", directory_type_str.c_str());
      return (DirectoryType) -1;
   }
}

DirectoryEntry*
Directory::createDirectoryEntry()
{
   switch (_directory_type)
   {
   case FULL_MAP:
      return new DirectoryEntryFullMap(_max_num_sharers);

   case LIMITED_NO_BROADCAST:
      return new DirectoryEntryLimitedNoBroadcast(_max_hw_sharers);

   case LIMITED_BROADCAST:
      return new DirectoryEntryLimitedBroadcast(_max_hw_sharers);

   case ACKWISE:
      return new DirectoryEntryAckwise(_max_hw_sharers);

   case LIMITLESS:
      return new DirectoryEntryLimitless(_max_hw_sharers, _max_num_sharers);

   default:
      LOG_PRINT_ERROR("Unrecognized Directory Type: %u", _directory_type);
      return NULL;
   }
}
