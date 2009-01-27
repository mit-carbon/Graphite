#include <cassert>

#include "network.h"
#include "network_types.h"

#include "network_model_magic.h"
#include "network_model_analytical.h"

NetworkModel *NetworkModel::createModel(Network *net, UInt32 type)
{
   switch (type)
   {
   case NETWORK_MAGIC:
      return new NetworkModelMagic(net);

   case NETWORK_ANALYTICAL_MESH:
      return new NetworkModelAnalytical(net);

   default:
      assert(false);
      return NULL;
   }
}
