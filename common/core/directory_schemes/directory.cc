using namespace std;

#include "simulator.h"
#include "directory.h"
#include "directory_entry.h"
#include "directory_entry_limited_broadcast.h"
#include "directory_entry_limited_no_broadcast.h"
#include "directory_entry_limitless.h"
#include "log.h"

Directory::Directory(string directory_type_str, UInt32 num_entries, UInt32 max_hw_sharers, UInt32 max_num_sharers):
   m_num_entries(num_entries),
   m_max_hw_sharers(max_hw_sharers),
   m_max_num_sharers(max_num_sharers)
{
   // Look at the type of directory and create 
   m_directory_entry_list = new DirectoryEntry*[m_num_entries];
  
   m_directory_type = parseDirectoryType(directory_type_str);
   for (UInt32 i = 0; i < m_num_entries; i++)
   {
      m_directory_entry_list[i] = createDirectoryEntry(m_directory_type);
   }
}

Directory::~Directory()
{
   for (UInt32 i = 0; i < m_num_entries; i++)
   {
      delete m_directory_entry_list[i];
   }
   delete [] m_directory_entry_list;
}

DirectoryEntry*
Directory::getDirectoryEntry(UInt32 entry_num)
{
   return m_directory_entry_list[entry_num]; 
}

Directory::DirectoryType
Directory::parseDirectoryType(string directory_type_str)
{
   if (directory_type_str == "limited_no_broadcast")
      return LIMITED_NO_BROADCAST;
   else if (directory_type_str == "limited_broadcast")
      return LIMITED_BROADCAST;
   else if (directory_type_str == "limitless")
      return LIMITLESS;
   else
   {
      LOG_PRINT_ERROR("Unsupported Directory Type: %s", directory_type_str.c_str());
      return (DirectoryType) -1;
   }
}

DirectoryEntry*
Directory::createDirectoryEntry(DirectoryType directory_type)
{
   switch (directory_type)
   {
      case LIMITED_NO_BROADCAST:
         return new DirectoryEntryLimitedNoBroadcast(m_max_hw_sharers, m_max_num_sharers);

      case LIMITED_BROADCAST:
         return new DirectoryEntryLimitedBroadcast(m_max_hw_sharers, m_max_num_sharers);

      case LIMITLESS:
         {
            UInt32 software_trap_penalty = 0;
            try
            {
               software_trap_penalty = Sim()->getCfg()->getInt("perf_model/dram_directory/limitless/software_trap_penalty");
            }
            catch (...)
            {
               LOG_PRINT_ERROR("Could not read 'cache_coherence/limitless/software_trap_penalty' from the config file");
            }
            return new DirectoryEntryLimitless(m_max_hw_sharers, m_max_num_sharers, software_trap_penalty);
         }

      default:
         LOG_PRINT_ERROR("Unrecognized Directory Type: %u", directory_type);
         return NULL;
   }
}
