#include "random_classifier.h"

namespace HybridProtocol_PPMOSI_SS
{

RandomClassifier::RandomClassifier(Granularity granularity)
   : Classifier()
   , _mode(Mode::PRIVATE)
   , _granularity(granularity)
{}

RandomClassifier::~RandomClassifier()
{}

Mode::Type
RandomClassifier::getMode(tile_id_t sharer)
{
   return _mode; 
}

void
RandomClassifier::updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry, BufferedReq* buffered_req)
{
   if (IS_BLOCKING_REQ(shmem_msg->getType()))
   {
      // Set mode to appropriate value
      UInt32 num = _rand_num.next(2);
      if (_granularity == LINE_BASED)
         _mode = (num == 0) ? Mode::PRIVATE : Mode::REMOTE_LINE;
      else // (_granularity == SHARER_BASED)
         _mode = (num == 0) ? Mode::PRIVATE : Mode::REMOTE_SHARER;

   }

}

}
