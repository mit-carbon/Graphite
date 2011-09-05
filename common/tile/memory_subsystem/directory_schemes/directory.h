#pragma once

#include <string>
using std::string;

#include "directory_entry.h"
#include "fixed_types.h"

class Directory
{
public:
   enum DirectoryType
   {
      FULL_MAP = 0,
      LIMITED_NO_BROADCAST,
      LIMITED_BROADCAST,
      ACKWISE,
      LIMITLESS,
      NUM_DIRECTORY_TYPES
   };

   Directory(string directory_type_str, SInt32 num_entries, SInt32 max_hw_sharers, SInt32 max_num_sharers);
   ~Directory();

   DirectoryEntry* getDirectoryEntry(SInt32 entry_num);
   void setDirectoryEntry(SInt32 entry_num, DirectoryEntry* directory_entry);
   UInt32 getDirectoryEntrySize();
   DirectoryEntry* createDirectoryEntry();
   
   static DirectoryType parseDirectoryType(std::string directory_type_str);

private:
   DirectoryType _directory_type;
   SInt32 _num_entries;
   SInt32 _max_hw_sharers;
   SInt32 _max_num_sharers;

   vector<DirectoryEntry*> _directory_entry_list;
};
