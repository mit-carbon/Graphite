#include "predictive_locality_based_classifier.h"
#include "utils.h"
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
PredictiveLocalityBasedClassifier::updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry, BufferedReq* buffered_req)
{
   tile_id_t requester = buffered_req ? buffered_req->getShmemMsg()->getRequester() : INVALID_TILE_ID;
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();
   
   if ((_tracked_sharer == INVALID_TILE_ID) && IS_BLOCKING_REQ(shmem_msg_type))
      _tracked_sharer = sender;
   if (_tracked_sharer != sender)
      return;

   switch (shmem_msg_type)
   {
   case ShmemMsg::UNIFIED_READ_REQ:
   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
   case ShmemMsg::WRITE_UNLOCK_REQ:
      {
         UInt32 remote_utilization = getUtilization();
         LOG_ASSERT_ERROR(remote_utilization <= _private_caching_threshold, "remote_utilization(%u), _private_caching_threshold(%u)",
                          remote_utilization, _private_caching_threshold);
         remote_utilization ++;
         setUtilization(remote_utilization);

         if (_mode == Mode::REMOTE_LINE)
         {
            // If req is a UNIFIED_READ_LOCK_REQ, switch to PRIVATE mode if utilization = (_private_caching_threshold-1)
            if ( ( (shmem_msg_type == ShmemMsg::UNIFIED_READ_LOCK_REQ) && (remote_utilization >= (_private_caching_threshold-1)) ) ||
                 (remote_utilization >= _private_caching_threshold) )
            {
               _mode = Mode::PRIVATE;
            }
         }
      }
      break;

   case ShmemMsg::INV_REPLY:
   case ShmemMsg::FLUSH_REPLY:
      {
         // Transition from PRIVATE -> REMOTE_LINE mode mostly
         // REMOTE_LINE -> PRIVATE transition may occur but is improbable
         UInt32 private_utilization = shmem_msg->getCacheLineUtilization();
         UInt32 remote_utilization = getUtilization();
         UInt32 utilization = private_utilization + remote_utilization;
         if (sender == requester)
         {
            setUtilization(utilization);
         }
         else // (sender != requester)
         {
            if (utilization >= _private_caching_threshold)
               _mode = Mode::PRIVATE;
            else // (utilization < _private_caching_threshold)
               _mode = Mode::REMOTE_LINE;
            // Set utilization to 0
            setUtilization(0);
            // Set the _tracked_sharer to INVALID_TILE_ID so that it is up for grabs again
            _tracked_sharer = INVALID_TILE_ID;
         }
      }
      break;

   default:
      // Do not handle the other types of messages
      break;
   }
}

UInt32
PredictiveLocalityBasedClassifier::getUtilization() const
{
   return _utilization;
}

void
PredictiveLocalityBasedClassifier::setUtilization(UInt32 utilization)
{
   _utilization = (utilization >= _private_caching_threshold) ? _private_caching_threshold : utilization;
}

void
PredictiveLocalityBasedClassifier::setPrivateCachingThreshold(UInt32 PCT)
{
   _private_caching_threshold = PCT;
}

}
