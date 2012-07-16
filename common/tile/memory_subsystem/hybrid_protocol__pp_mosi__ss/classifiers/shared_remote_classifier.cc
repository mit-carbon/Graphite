#include "shared_remote_classifier.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

SharedRemoteClassifier::SharedRemoteClassifier()
   : Classifier()
   , _saved_mode(Mode::PRIVATE)
{}

SharedRemoteClassifier::~SharedRemoteClassifier()
{}

Mode::Type
SharedRemoteClassifier::getMode(tile_id_t sharer)
{
   return _saved_mode;
}

void
SharedRemoteClassifier::updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry, BufferedReq* buffered_req)
{
   if (_saved_mode == Mode::REMOTE_LINE)
      return;
   
   if (!IS_BLOCKING_REQ(shmem_msg->getType()))
      return;

   tile_id_t sharer = sender;
   UInt32 num_sharers = directory_entry->getNumSharers();

   // For new cache lines
   if (num_sharers == 0)
      return;

   LOG_ASSERT_ERROR(num_sharers <= 1, "num_sharers(%i)", num_sharers);
   
   // For private cache lines
   if ( (num_sharers == 1) && (directory_entry->hasSharer(sharer)) )
      return;

   // Set the line in remote mode
   _saved_mode = Mode::REMOTE_LINE;
}

}
