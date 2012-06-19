#pragma once

#include "directory_entry_full_map.h"
#include "../adaptive_directory_entry.h"

namespace HybridProtocol_PPMOSI_SS
{

class AdaptiveDirectoryEntryFullMap : public DirectoryEntryFullMap, public AdaptiveDirectoryEntry
{
public:
   AdaptiveDirectoryEntryFullMap(SInt32 max_hw_sharers)
      : DirectoryEntryFullMap(max_hw_sharers)
      , AdaptiveDirectoryEntry(max_hw_sharers)
   {}
   ~AdaptiveDirectoryEntryFullMap()
   {}
};

}
