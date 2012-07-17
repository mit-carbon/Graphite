#include "locality_based_classifier.h"
#include "log.h"
#include "utils.h"

namespace HybridProtocol_PPMOSI_SS
{

UInt32 LocalityBasedClassifier::_private_caching_threshold;
UInt32 LocalityBasedClassifier::_num_utilization_bits;

LocalityBasedClassifier::LocalityBasedClassifier(SInt32 max_hw_sharers)
   : Classifier()
{
   _utilization_vec = new SmallDataVector(max_hw_sharers, _num_utilization_bits);
   _mode_vec = new SmallDataVector(max_hw_sharers, 1);
}

LocalityBasedClassifier::~LocalityBasedClassifier()
{
   delete _utilization_vec;
   delete _mode_vec;
}

void
LocalityBasedClassifier::updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry, BufferedReq* buffered_req)
{
   tile_id_t requester = buffered_req ? buffered_req->getShmemMsg()->getRequester() : INVALID_TILE_ID;
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();
   switch (shmem_msg_type)
   {
   case ShmemMsg::UNIFIED_READ_REQ:
   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
   case ShmemMsg::WRITE_UNLOCK_REQ:
      {
         // REMOTE_SHARER -> PRIVATE transition
         // Transition can never happen on a WRITE_UNLOCK_REQ to directory (since cache line is locked)
         // (remote_utilization == _private_caching_threshold) is for a WRITE_UNLOCK_REQ that must really cause a transition
         Mode::Type mode = getMode(sender);
         
         UInt32 remote_utilization = getUtilization(sender);
         LOG_ASSERT_ERROR(remote_utilization <= _private_caching_threshold, "remote_utilization(%u), _private_caching_threshold(%u)",
                          remote_utilization, _private_caching_threshold);
         remote_utilization ++;
         setUtilization(sender, remote_utilization);
         
         if (mode == Mode::REMOTE_SHARER)
         {
            // If req is a UNIFIED_READ_LOCK_REQ, switch to PRIVATE mode if utilization = (_private_caching_threshold-1)
            if ( ( (shmem_msg_type == ShmemMsg::UNIFIED_READ_LOCK_REQ) && (remote_utilization >= (_private_caching_threshold-1)) ) ||
                 (remote_utilization >= _private_caching_threshold) )
            {
               setMode(sender, Mode::PRIVATE);
            }
         }
      }
      break;

   case ShmemMsg::INV_REPLY:
   case ShmemMsg::FLUSH_REPLY:
      {
         // PRIVATE -> REMOTE_SHARER transition mostly
         // REMOTE_SHARER -> PRIVATE transition may occur but is improbable
         UInt32 private_utilization = shmem_msg->getCacheLineUtilization();
         UInt32 remote_utilization = getUtilization(sender);
         UInt32 utilization = private_utilization + remote_utilization;
         if (sender == requester)
         {
            setUtilization(sender, utilization);
         }
         else // (sender != requester)
         {
            if (utilization >= _private_caching_threshold)
               setMode(sender, Mode::PRIVATE);
            else // (utilization < _private_caching_threshold)
               setMode(sender, Mode::REMOTE_SHARER);
            // Set utilization to 0
            setUtilization(sender, 0);
         }
      }
      break;

   default:
      // Don't need to do any updates
      break;
   }
}

Mode::Type
LocalityBasedClassifier::getMode(tile_id_t sharer)
{
   UInt32 mode_type = _mode_vec->get(sharer);
   return (mode_type == 0) ? Mode::PRIVATE : Mode::REMOTE_SHARER;
}

void
LocalityBasedClassifier::setMode(tile_id_t sharer, Mode::Type mode)
{
   UInt32 mode_type = (mode == Mode::PRIVATE) ? 0 : 1;
   _mode_vec->set(sharer, mode_type);
}

UInt32
LocalityBasedClassifier::getUtilization(tile_id_t sharer)
{
   return _utilization_vec->get(sharer);
}

void
LocalityBasedClassifier::setUtilization(tile_id_t sharer, UInt32 utilization)
{
   if (utilization >= _private_caching_threshold)
      utilization = _private_caching_threshold;
   _utilization_vec->set(sharer, utilization);
}

void
LocalityBasedClassifier::setPrivateCachingThreshold(UInt32 PCT)
{
   LOG_ASSERT_ERROR(PCT >= 1, "Private Caching Threshold must be >= 1");
   _private_caching_threshold = PCT;
   _num_utilization_bits = floorLog2(PCT)+1;
}

}
