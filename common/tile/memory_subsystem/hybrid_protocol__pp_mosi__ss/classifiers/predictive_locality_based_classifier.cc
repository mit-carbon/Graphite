#include "predictive_locality_based_classifier.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

UInt32 PredictiveLocalityBasedClassifier::_private_caching_threshold;

PredictiveLocalityBasedClassifier::PredictiveLocalityBasedClassifier()
   : Classifier()
   , _mode(Mode::PRIVATE)
   , _utilization(0)
   , _tracked_sharer(INVALID_TILE_ID)
{}

PredictiveLocalityBasedClassifier::~PredictiveLocalityBasedClassifier()
{}

Mode::Type
PredictiveLocalityBasedClassifier::getMode(tile_id_t sharer)
{
   // Use the locality information of one sharer to predict the locality of all other sharers
   return _mode;
}

void
PredictiveLocalityBasedClassifier::updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry)
{
   if (_tracked_sharer == INVALID_TILE_ID)
      _tracked_sharer = sender;
   if (_tracked_sharer != sender)
      return;

   switch (shmem_msg->getType())
   {
   case ShmemMsg::UNIFIED_READ_REQ:
   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
   case ShmemMsg::WRITE_UNLOCK_REQ:
      _utilization ++;
      _mode = (_utilization >= _private_caching_threshold) ? Mode::PRIVATE: Mode::REMOTE_LINE;
      break;

   case ShmemMsg::INV_REPLY:
   case ShmemMsg::WB_REPLY:
   case ShmemMsg::FLUSH_REPLY:
      _utilization = shmem_msg->getCacheLineUtilization();
      _mode = (_utilization >= _private_caching_threshold) ? Mode::PRIVATE: Mode::REMOTE_LINE;
      break;

   default:
      // Do not handle the other types of messages
      break;
   }
}

void
PredictiveLocalityBasedClassifier::setPrivateCachingThreshold(UInt32 PCT)
{
   _private_caching_threshold = PCT;
}

}
