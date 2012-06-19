#pragma once

#include "../classifier.h"

namespace HybridProtocol_PPMOSI_SS
{

class SharedRemoteClassifier : public Classifier
{
public:
   SharedRemoteClassifier();
   ~SharedRemoteClassifier();

   Mode getMode(tile_id_t sharer, ShmemMsg::Type req_type, DirectoryEntry* directory_entry);
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry);

private:
   Mode _saved_mode;
};

}
