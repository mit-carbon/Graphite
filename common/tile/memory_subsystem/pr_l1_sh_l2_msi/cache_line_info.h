#pragma once

#include "../cache/cache_line_info.h"
#include "cache_level.h"
#include "directory_entry.h"

namespace PrL1ShL2MSI
{

CacheLineInfo* createCacheLineInfo(SInt32 cache_level);

typedef CacheLineInfo PrL1CacheLineInfo;

class ShL2CacheLineInfo : public CacheLineInfo
{
public:
   ShL2CacheLineInfo(IntPtr tag = ~0, DirectoryEntry* directory_entry = NULL);
   ~ShL2CacheLineInfo();

   void assign(CacheLineInfo* cache_line_info);
   
   DirectoryEntry* getDirectoryEntry() const
   { return _directory_entry; }
   void setDirectoryEntry(DirectoryEntry* entry)
   { _directory_entry = entry; }
   MemComponent::Type getCachingComponent() const
   { return _caching_component; }
   void setCachingComponent(MemComponent::Type caching_component)
   { _caching_component = caching_component; }

private:
   DirectoryEntry* _directory_entry;
   MemComponent::Type _caching_component;
};

}
