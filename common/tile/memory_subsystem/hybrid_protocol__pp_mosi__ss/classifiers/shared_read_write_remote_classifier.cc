#include <sstream>
using std::ostringstream;
#include "shared_read_write_remote_classifier.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

SharedReadWriteRemoteClassifier::SharedReadWriteRemoteClassifier()
   : Classifier()
   , _saved_mode(Mode::PRIVATE)
{}

SharedReadWriteRemoteClassifier::~SharedReadWriteRemoteClassifier()
{}

Mode::Type
SharedReadWriteRemoteClassifier::getMode(tile_id_t sharer)
{
   return _saved_mode;
}

void
SharedReadWriteRemoteClassifier::updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry, BufferedReq* buffered_req)
{
   if (_saved_mode == Mode::REMOTE_LINE)
      return;
   
   if (!IS_BLOCKING_REQ(shmem_msg->getType()))
      return;

   tile_id_t sharer = sender;
   UInt32 num_sharers = directory_entry->getNumSharers();
   DirectoryState::Type dstate = directory_entry->getDirectoryBlockInfo()->getDState();
   ShmemMsg::Type req_type = shmem_msg->getType();

   // For new cache lines
   if (num_sharers == 0)
      return;

   // For private cache lines
   if ( (num_sharers == 1) && (directory_entry->hasSharer(sharer)) )
      return;

   // If state is MODIFIED or OWNED, there has been a write to it, so return in REMOTE mode
   if ( (dstate == DirectoryState::MODIFIED) || (dstate == DirectoryState::OWNED) )
   {
      _saved_mode = Mode::REMOTE_LINE;
      return;
   }
   
   // Now, line is in SHARED or EXCLUSIVE state
   // For read-only cache lines
   if (req_type == ShmemMsg::UNIFIED_READ_REQ)
      return;

   // Set the line in remote mode
   _saved_mode = Mode::REMOTE_LINE;
}

}
