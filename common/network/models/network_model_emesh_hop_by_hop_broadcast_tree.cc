#include "network_model_emesh_hop_by_hop_broadcast_tree.h"
#include "simulator.h"
#include "config.h"

NetworkModelEMeshHopByHopBroadcastTree::NetworkModelEMeshHopByHopBroadcastTree(Network* net, SInt32 network_id):
   NetworkModelEMeshHopByHopGeneric(net, network_id)
{
   m_broadcast_tree_enabled = true;
   
   try
   {
      // Network Frequency is specified in GHz
      m_frequency = Sim()->getCfg()->getFloat("network/emesh_hop_by_hop_broadcast_tree/frequency");
      // Link Width is specified in bits
      m_link_width = Sim()->getCfg()->getInt("network/emesh_hop_by_hop_broadcast_tree/link/width");
      // Link Length in mm
      m_link_length = Sim()->getCfg()->getFloat("network/emesh_hop_by_hop_broadcast_tree/link/length");
      // Link Type
      m_link_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop_broadcast_tree/link/type");
      // FIXME: Check if this is a valid modeling of the broadcast tree network (look at VCT paper)
      // Router Delay is specified in cycles
      m_router_delay = (UInt64) Sim()->getCfg()->getInt("network/emesh_hop_by_hop_broadcast_tree/router/delay");
      // Number of flits per port
      // Used for power modeling purposes now - should be used later for performance modeling
      m_num_flits_per_output_buffer = Sim()->getCfg()->getInt("network/emesh_hop_by_hop_broadcast_tree/router/num_flits_per_port_buffer");

      // Queue Model enabled? If no, this degrades into a hop counter model
      m_queue_model_enabled = Sim()->getCfg()->getBool("network/emesh_hop_by_hop_broadcast_tree/queue_model/enabled");
      m_queue_model_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop_broadcast_tree/queue_model/type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read emesh_hop_by_hop_broadcast_tree parameters from the configuration file");
   }

   initializeModels();
}
