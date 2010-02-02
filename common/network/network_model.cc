#include <cassert>

#include "network.h"
#include "network_types.h"

#include "network_model_magic.h"
#include "network_model_hop_counter.h"
#include "network_model_analytical.h"
#include "network_model_emesh_hop_by_hop.h"
#include "network_model_atac_optical_bus.h"
#include "log.h"

NetworkModel*
NetworkModel::createModel(Network *net, UInt32 model_type, EStaticNetwork net_type)
{
   switch (model_type)
   {
   case NETWORK_MAGIC:
      return new NetworkModelMagic(net);

   case NETWORK_HOP_COUNTER:
      return new NetworkModelHopCounter(net);

   case NETWORK_ANALYTICAL_MESH:
      return new NetworkModelAnalytical(net, net_type);

   case NETWORK_EMESH_HOP_BY_HOP:
      return new NetworkModelEMeshHopByHop(net);

   case NETWORK_ATAC_OPTICAL_BUS:
      return new NetworkModelAtacOpticalBus(net);

   default:
      assert(false);
      return NULL;
   }
}

UInt32 
NetworkModel::parseNetworkType(std::string str)
{
   if (str == "magic")
      return NETWORK_MAGIC;
   else if (str == "hop_counter")
      return NETWORK_HOP_COUNTER;
   else if (str == "analytical")
      return NETWORK_ANALYTICAL_MESH;
   else if (str == "emesh_hop_by_hop")
      return NETWORK_EMESH_HOP_BY_HOP;
   else if (str == "atac_optical_bus")
      return NETWORK_ATAC_OPTICAL_BUS;
   else
      return (UInt32)-1;
}

std::pair<bool,SInt32> 
NetworkModel::computeCoreCountConstraints(UInt32 network_type, SInt32 core_count)
{
   switch (network_type)
   {
      case NETWORK_MAGIC:
      case NETWORK_HOP_COUNTER:
      case NETWORK_ANALYTICAL_MESH:
      case NETWORK_ATAC_OPTICAL_BUS:
         return std::make_pair(false,core_count);

      case NETWORK_EMESH_HOP_BY_HOP:
         return NetworkModelEMeshHopByHop::computeCoreCountConstraints(core_count);

      default:
         LOG_PRINT_ERROR("Unrecognized network type(%u)", network_type);
         return std::make_pair(false,-1);
   }
}
