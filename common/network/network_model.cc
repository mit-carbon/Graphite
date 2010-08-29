#include <cassert>
using namespace std;

#include "network.h"
#include "network_types.h"

#include "network_model_magic.h"
#include "network_model_emesh_hop_counter.h"
#include "network_model_analytical.h"
#include "network_model_emesh_hop_by_hop_basic.h"
#include "network_model_emesh_hop_by_hop_broadcast_tree.h"
#include "network_model_atac_optical_bus.h"
#include "network_model_atac_cluster.h"
#include "log.h"

NetworkModel::NetworkModel(Network *network, SInt32 network_id):
   _network(network),
   _network_id(network_id)
{
   if (network_id == 0)
      _network_name = "network/user_model_1";
   else if (network_id == 1)
      _network_name = "network/user_model_2";
   else if (network_id == 2)
      _network_name = "network/memory_model_1";
   else if (network_id == 3)
      _network_name = "network/memory_model_2";
   else if (network_id == 4)
      _network_name = "network/system_model";
   else
      LOG_PRINT_ERROR("Unrecognized Network Num(%u)", network_id);
}

NetworkModel*
NetworkModel::createModel(Network *net, SInt32 network_id, UInt32 model_type)
{
   switch (model_type)
   {
   case NETWORK_MAGIC:
      return new NetworkModelMagic(net, network_id);

   case NETWORK_EMESH_HOP_COUNTER:
      return new NetworkModelEMeshHopCounter(net, network_id);

   case NETWORK_ANALYTICAL_MESH:
      return new NetworkModelAnalytical(net, network_id);

   case NETWORK_EMESH_HOP_BY_HOP_BASIC:
      return new NetworkModelEMeshHopByHopBasic(net, network_id);

   case NETWORK_EMESH_HOP_BY_HOP_BROADCAST_TREE:
      return new NetworkModelEMeshHopByHopBroadcastTree(net, network_id);

   case NETWORK_ATAC_OPTICAL_BUS:
      return new NetworkModelAtacOpticalBus(net, network_id);

   case NETWORK_ATAC_CLUSTER:
      return new NetworkModelAtacCluster(net, network_id);

   default:
      LOG_PRINT_ERROR("Unrecognized Network Model(%u)", model_type);
      return NULL;
   }
}

UInt32 
NetworkModel::parseNetworkType(string str)
{
   if (str == "magic")
      return NETWORK_MAGIC;
   else if (str == "emesh_hop_counter")
      return NETWORK_EMESH_HOP_COUNTER;
   else if (str == "analytical")
      return NETWORK_ANALYTICAL_MESH;
   else if (str == "emesh_hop_by_hop_basic")
      return NETWORK_EMESH_HOP_BY_HOP_BASIC;
   else if (str == "emesh_hop_by_hop_broadcast_tree")
      return NETWORK_EMESH_HOP_BY_HOP_BROADCAST_TREE;
   else if (str == "atac_optical_bus")
      return NETWORK_ATAC_OPTICAL_BUS;
   else if (str == "atac_cluster")
      return NETWORK_ATAC_CLUSTER;
   else
      return (UInt32)-1;
}

pair<bool,SInt32> 
NetworkModel::computeCoreCountConstraints(UInt32 network_type, SInt32 core_count)
{
   switch (network_type)
   {
      case NETWORK_MAGIC:
      case NETWORK_EMESH_HOP_COUNTER:
      case NETWORK_ANALYTICAL_MESH:
      case NETWORK_ATAC_OPTICAL_BUS:
         return make_pair(false,core_count);

      case NETWORK_ATAC_CLUSTER:
      case NETWORK_EMESH_HOP_BY_HOP_BASIC:
      case NETWORK_EMESH_HOP_BY_HOP_BROADCAST_TREE:
         return NetworkModelEMeshHopByHopGeneric::computeCoreCountConstraints(core_count);

      default:
         fprintf(stderr, "Unrecognized network type(%u)\n", network_type);
         assert(false);
         return make_pair(false,-1);
   }
}

pair<bool, vector<core_id_t> > 
NetworkModel::computeMemoryControllerPositions(UInt32 network_type, SInt32 num_memory_controllers, SInt32 core_count)
{
   switch(network_type)
   {
      case NETWORK_MAGIC:
      case NETWORK_EMESH_HOP_COUNTER:
      case NETWORK_ANALYTICAL_MESH:
      case NETWORK_ATAC_OPTICAL_BUS:
         {
            SInt32 spacing_between_memory_controllers = core_count / num_memory_controllers;
            vector<core_id_t> core_list_with_memory_controllers;
            for (core_id_t i = 0; i < num_memory_controllers; i++)
            {
               assert((i*spacing_between_memory_controllers) < core_count);
               core_list_with_memory_controllers.push_back(i * spacing_between_memory_controllers);
            }
            
            return make_pair(false, core_list_with_memory_controllers);
         }

      case NETWORK_EMESH_HOP_BY_HOP_BASIC:
      case NETWORK_EMESH_HOP_BY_HOP_BROADCAST_TREE:
         return NetworkModelEMeshHopByHopGeneric::computeMemoryControllerPositions(num_memory_controllers, core_count);

      case NETWORK_ATAC_CLUSTER:
         return NetworkModelAtacCluster::computeMemoryControllerPositions(num_memory_controllers, core_count);

      default:
         LOG_PRINT_ERROR("Unrecognized network type(%u)", network_type);
         return make_pair(false, vector<core_id_t>());
   }
}

pair<bool, vector<Config::CoreList> >
NetworkModel::computeProcessToCoreMapping(UInt32 network_type)
{
   switch(network_type)
   {
      case NETWORK_MAGIC:
      case NETWORK_ANALYTICAL_MESH:
      case NETWORK_EMESH_HOP_COUNTER:
      case NETWORK_ATAC_OPTICAL_BUS:
         return make_pair(false, vector<vector<core_id_t> >());

      case NETWORK_EMESH_HOP_BY_HOP_BASIC:
      case NETWORK_EMESH_HOP_BY_HOP_BROADCAST_TREE:
         return NetworkModelEMeshHopByHopGeneric::computeProcessToCoreMapping();

      case NETWORK_ATAC_CLUSTER:
         return NetworkModelAtacCluster::computeProcessToCoreMapping();

      default:
         fprintf(stderr, "Unrecognized network type(%u)\n", network_type);
         assert(false);
         return make_pair(false, vector<vector<core_id_t> >());
   }
}
