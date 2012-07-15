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

Mode::Type
LocalityBasedClassifier::getMode(tile_id_t sharer)
{
   UInt32 mode_type = _mode_vec->get(sharer);
   return (mode_type == 0) ? Mode::PRIVATE : Mode::REMOTE_SHARER;
}

void
LocalityBasedClassifier::updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry)
{
   switch (shmem_msg->getType())
   {
   case ShmemMsg::UNIFIED_READ_REQ:
   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
   case ShmemMsg::WRITE_UNLOCK_REQ:
      setUtilization(sender, getUtilization(sender)+1);
      updateMode(sender);
      break;

   case ShmemMsg::INV_REPLY:
   case ShmemMsg::WB_REPLY:
   case ShmemMsg::FLUSH_REPLY:
      setUtilization(sender, shmem_msg->getCacheLineUtilization());
      updateMode(sender);
      break;

   default:
      // Don't need to do any updates
      break;
   }
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
   _utilization_vec->set(sharer, utilization);
}

void
LocalityBasedClassifier::updateMode(tile_id_t sharer)
{
   UInt32 utilization = _utilization_vec->get(sharer);
   if (utilization >= _private_caching_threshold)
   {
      // PRIVATE_SHARER_MODE
      setMode(sharer, Mode::PRIVATE);
   }
   else // (utilization < _private_caching_threshold)
   {
      // REMOTE_SHARER_MODE
      setMode(sharer, Mode::REMOTE_SHARER);
   }
}

void
LocalityBasedClassifier::setPrivateCachingThreshold(UInt32 PCT)
{
   _private_caching_threshold = PCT;
   _num_utilization_bits = floorLog2(PCT)+1;
}

}
