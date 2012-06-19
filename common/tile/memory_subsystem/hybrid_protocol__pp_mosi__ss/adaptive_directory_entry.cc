#include "adaptive_directory_entry/full_map.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

AdaptiveDirectoryEntry::AdaptiveDirectoryEntry(SInt32 max_hw_sharers)
{
   _classifier = Classifier::create(max_hw_sharers);
}

AdaptiveDirectoryEntry::~AdaptiveDirectoryEntry()
{
   delete _classifier;
}

DirectoryEntry* createAdaptiveDirectoryEntry(DirectoryType directory_type, SInt32 max_hw_sharers, SInt32 max_num_sharers)
{
   if (Classifier::getType() == Classifier::LOCALITY_BASED)
      LOG_ASSERT_ERROR(directory_type == FULL_MAP, "Classifier \"per_sharer_locality_based\" only supported with a \"full_map\" directory");

   switch (directory_type)
   {
   case FULL_MAP:
      return new AdaptiveDirectoryEntryFullMap(max_num_sharers);

   default:
      LOG_PRINT_ERROR("Only full_map directory supported at the moment");
      return (DirectoryEntry*) NULL;
   }
}

}
