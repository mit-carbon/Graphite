#pragma once

#include "../classifier.h"
#include "common/misc/random.h"

namespace HybridProtocol_PPMOSI_SS
{

class RandomClassifier : public Classifier
{
public:
   RandomClassifier() : Classifier() {}
   ~RandomClassifier() {}

   Mode getMode(tile_id_t sharer, ShmemMsg::Type req_type, DirectoryEntry* directory_entry)
   { return ((bool) _rand_num.next(2)) ? PRIVATE_MODE : REMOTE_MODE; }
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry)
   {}

private:
   Random _rand_num;
};

}
