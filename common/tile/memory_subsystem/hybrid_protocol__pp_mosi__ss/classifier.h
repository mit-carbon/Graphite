#pragma once

#include <string>
using std::string;

#include "mode.h"
#include "fixed_types.h"
#include "shmem_msg.h"
#include "directory_entry.h"
#include "buffered_req.h"

namespace HybridProtocol_PPMOSI_SS
{

class Classifier
{
public:
   enum Type
   {
      PRIVATE_FIXED,
      REMOTE_FIXED,
      RANDOM_LINE_BASED,
      RANDOM_SHARER_BASED,
      SHARED_REMOTE,
      SHARED_READ_WRITE_REMOTE,
      LOCALITY_BASED,
      PREDICTIVE_LOCALITY_BASED,
      NUM_TYPES
   };
   enum Granularity
   {
      LINE_BASED,
      SHARER_BASED
   };

   Classifier();
   virtual ~Classifier();

   static Type getType()            { return _type; }
   static void setType(Type type)   { _type = type; }
   static Type parse(string type);

   static Classifier* create(SInt32 max_hw_sharers);

   virtual Mode::Type getMode(tile_id_t sharer) = 0;
   virtual void updateMode(tile_id_t sender, ShmemMsg* shmem_msg, DirectoryEntry* directory_entry, BufferedReq* buffered_req) = 0;

private:
   static Type _type;
};

}
