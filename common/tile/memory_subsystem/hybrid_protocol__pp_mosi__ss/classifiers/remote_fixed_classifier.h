#pragma once

#include "../classifier.h"

namespace HybridProtocol_PPMOSI_SS
{

class RemoteFixedClassifier : public Classifier
{
public:
   RemoteFixedClassifier() : Classifier() {}
   ~RemoteFixedClassifier() {}

   Mode getMode(tile_id_t sharer, ShmemMsg::Type req_type, DirectoryEntry* directory_entry)
   { return REMOTE_MODE; }
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry)
   {}
};

}
