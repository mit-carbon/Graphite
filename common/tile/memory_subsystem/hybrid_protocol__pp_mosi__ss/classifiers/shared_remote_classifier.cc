#include "shared_remote_classifier.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

SharedRemoteClassifier::SharedRemoteClassifier()
   : Classifier()
   , _saved_mode(INVALID_MODE)
{}

SharedRemoteClassifier::~SharedRemoteClassifier()
{}

Mode
SharedRemoteClassifier::getMode(tile_id_t sharer, ShmemMsg::Type req_type, DirectoryEntry* directory_entry)
{
   return (_saved_mode == INVALID_MODE) ? PRIVATE_MODE : REMOTE_MODE;
}

void
SharedRemoteClassifier::updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry)
{
   if ((shmem_msg->getType() < ShmemMsg::UNIFIED_READ_REQ) || (shmem_msg->getType() > ShmemMsg::WRITE_UNLOCK_REQ))
      return;

   tile_id_t sharer = sender;
   if (_saved_mode != INVALID_MODE)
      return;

   SInt32 num_sharers = directory_entry->getNumSharers();
   LOG_ASSERT_ERROR(num_sharers <= 1, "num_sharers(%i)", num_sharers);

   if (num_sharers == 1)
   {
      tile_id_t previous_sharer = directory_entry->getOneSharer();
      if (previous_sharer != sharer)
         _saved_mode = REMOTE_MODE;
   }
}

}
