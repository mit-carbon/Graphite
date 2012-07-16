#pragma once

#include "../classifier.h"

namespace HybridProtocol_PPMOSI_SS
{

class SharedReadWriteRemoteClassifier : public Classifier
{
public:
   SharedReadWriteRemoteClassifier();
   ~SharedReadWriteRemoteClassifier();

   Mode::Type getMode(tile_id_t sharer);
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry, BufferedReq* buffered_req);

private:
   Mode::Type _saved_mode;
};

}
