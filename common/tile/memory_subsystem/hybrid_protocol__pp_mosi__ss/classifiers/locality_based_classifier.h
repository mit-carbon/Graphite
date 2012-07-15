#pragma once

#include "../classifier.h"
#include "small_data_vector.h"

namespace HybridProtocol_PPMOSI_SS
{

class LocalityBasedClassifier : public Classifier
{
public:
   LocalityBasedClassifier(SInt32 max_hw_sharers);
   ~LocalityBasedClassifier();

   Mode::Type getMode(tile_id_t sharer);
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry);
   static void setPrivateCachingThreshold(UInt32 PCT);
   
private:
   static UInt32 _private_caching_threshold;
   static UInt32 _num_utilization_bits;
   SmallDataVector* _utilization_vec;
   SmallDataVector* _mode_vec;
   
   void setMode(tile_id_t sharer, Mode::Type mode);
   UInt32 getUtilization(tile_id_t sharer);
   void setUtilization(tile_id_t sharer, UInt32 utilization);
   void updateMode(tile_id_t sharer);
};

}
