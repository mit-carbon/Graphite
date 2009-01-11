#include <cassert>

#include "network.h"
#include "network_types.h"

#include "network_model_magic.h"

NetworkModel *NetworkModel::createModel(Network *net, UInt32 type)
{
   switch (type)
   {
   case NETWORK_MAGIC:
      return new NetworkModelMagic(net);

   default:
      assert(false);
      return NULL;
   }
}
