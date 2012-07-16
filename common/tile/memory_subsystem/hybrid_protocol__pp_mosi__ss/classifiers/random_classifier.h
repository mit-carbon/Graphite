#pragma once

#include "../classifier.h"
#include "common/misc/random.h"

namespace HybridProtocol_PPMOSI_SS
{

class RandomClassifier : public Classifier
{
public:
   RandomClassifier(Granularity granularity);
   ~RandomClassifier();

   Mode::Type getMode(tile_id_t sharer);
   void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry, BufferedReq* buffered_req);

private:
   Random _rand_num;
   Mode::Type _mode;
   Granularity _granularity;
};

}
