#pragma once

#include "../classifier.h"

namespace HybridProtocol_PPMOSI_SS
{

class RemoteFixedClassifier : public Classifier
{
public:
   RemoteFixedClassifier() : Classifier() {}
   ~RemoteFixedClassifier() {}

   Mode::Type getMode(tile_id_t sharer)
   { return Mode::REMOTE_LINE; }
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry)
   {}
};

}
