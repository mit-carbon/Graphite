#pragma once

#include "../classifier.h"

namespace HybridProtocol_PPMOSI_SS
{

class PrivateFixedClassifier : public Classifier
{
public:
   PrivateFixedClassifier() : Classifier() {}
   ~PrivateFixedClassifier() {}

   Mode getMode(tile_id_t sharer, ShmemMsg::Type req_type, DirectoryEntry* directory_entry)
   { return PRIVATE_MODE; }
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry)
   {}
};

}
