#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

#include <string>

#include "directory_entry.h"
#include "fixed_types.h"

class Directory
{
   public:
      enum DirectoryType
      {
         LIMITED_NO_BROADCAST = 0,
         LIMITED_BROADCAST,
         LIMITLESS,
         NUM_DIRECTORY_TYPES
      };

   private:
      DirectoryType m_directory_type;
      UInt32 m_num_entries;
      UInt32 m_max_hw_sharers;
      UInt32 m_max_num_sharers;

      DirectoryEntry** m_directory_entry_list;

   public:
      Directory(std::string directory_type_str, UInt32 num_entries, UInt32 max_hw_sharers, UInt32 max_num_sharers);
      ~Directory();

      DirectoryEntry* getDirectoryEntry(UInt32 entry_num);
      DirectoryType parseDirectoryType(std::string directory_type_str);
      DirectoryEntry* createDirectoryEntry(DirectoryType directory_type);
};

#endif /* __DIRECTORY_H__ */
