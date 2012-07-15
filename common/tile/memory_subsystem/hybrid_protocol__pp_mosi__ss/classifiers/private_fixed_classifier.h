#pragma once

#include "../classifier.h"

namespace HybridProtocol_PPMOSI_SS
{

class PrivateFixedClassifier : public Classifier
{
public:
   PrivateFixedClassifier() : Classifier() {}
   ~PrivateFixedClassifier() {}

   Mode::Type getMode(tile_id_t sharer)
   { return Mode::PRIVATE; }
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry)
   {}
};

}
