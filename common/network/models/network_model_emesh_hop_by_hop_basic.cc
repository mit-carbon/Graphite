#include "network_model_emesh_hop_by_hop_basic.h"
#include "simulator.h"
#include "config.h"

NetworkModelEMeshHopByHopBasic::NetworkModelEMeshHopByHopBasic(Network* net, SInt32 network_id):
   NetworkModelEMeshHopByHopGeneric(net, network_id)
{
   m_broadcast_tree_enabled = false;

   try
   {
      // Network Frequency is specified in GHz
      m_frequency = Sim()->getCfg()->getFloat("network/emesh_hop_by_hop_basic/frequency");
      // Link Width is specified in bits
      m_link_width = Sim()->getCfg()->getInt("network/emesh_hop_by_hop_basic/link/width");
      // Link Length in mm
      m_link_length = Sim()->getCfg()->getFloat("network/emesh_hop_by_hop_basic/link/length");
      // Link Type
      m_link_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop_basic/link/type");
      // Router Delay (pipeline delay) is specified in cycles
      m_router_delay = (UInt64) Sim()->getCfg()->getInt("network/emesh_hop_by_hop_basic/router/delay");
      // Number of flits per port - Used for power modeling purposes now - Should be used later for performance modeling
      m_num_flits_per_output_buffer = Sim()->getCfg()->getInt("network/emesh_hop_by_hop_basic/router/num_flits_per_port_buffer");

      // Queue Model enabled? If no, this degrades into a hop counter model
      m_queue_model_enabled = Sim()->getCfg()->getBool("network/emesh_hop_by_hop_basic/queue_model/enabled");
      m_queue_model_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop_basic/queue_model/type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read emesh_hop_by_hop_basic parameters from the configuration file");
   }

   initializeModels();
}
