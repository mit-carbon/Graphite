#pragma once

#include "../classifier.h"

namespace HybridProtocol_PPMOSI_SS
{

class SharedRemoteClassifier : public Classifier
{
public:
   SharedRemoteClassifier();
   ~SharedRemoteClassifier();

   Mode::Type getMode(tile_id_t sharer);
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry);

private:
   Mode::Type _saved_mode;
};

}
