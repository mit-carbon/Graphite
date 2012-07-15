#pragma once

#include "../classifier.h"

namespace HybridProtocol_PPMOSI_SS
{

class PredictiveLocalityBasedClassifier : public Classifier
{
public:
   PredictiveLocalityBasedClassifier();
   ~PredictiveLocalityBasedClassifier();

   Mode::Type getMode(tile_id_t sharer);
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry);
   static void setPrivateCachingThreshold(UInt32 PCT);

private:
   static UInt32 _private_caching_threshold;
   Mode::Type _mode;
   UInt32 _utilization;
   tile_id_t _tracked_sharer;
};

}
