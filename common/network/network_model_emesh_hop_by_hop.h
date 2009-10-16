#ifndef __NETWORK_MODEL_EMESH_HOP_BY_HOP_H__
#define __NETWORK_MODEL_EMESH_HOP_BY_HOP_H__

#include "network.h"

class NetworkModelEMeshHopByHop : public NetworkModel
{
   public:
      typedef enum
      {
         UP = 0,
         DOWN,
         LEFT,
         RIGHT,
         NUM_OUTPUT_DIRECTIONS,
         SELF  // Does not have a queue corresponding to this
      } OutputDirection;

   private:
      // Fields
      SInt32 m_mesh_width;
      SInt32 m_mesh_height;
      float m_link_bandwidth;
      UInt64 m_hop_latency;
      bool m_broadcast_tree_enabled;

      bool m_queue_model_enabled;
      bool m_enabled;

      // Counters
      UInt64 m_bytes_sent;

      // Functions
      void computePosition(core_id_t core, SInt32 &x, SInt32 &y);
      core_id_t computeCoreId(SInt32 x, SInt32 y);
      void addHop(OutputDirection& direction, core_id_t final_dest, core_id_t next_dest, UInt64 pkt_time, UInt64 pkt_length, std::vector<Hop>& nextHops);
      UInt64 computeLatency(Direction direction, UInt64 time, UInt64 pkt_size);
      core_id_t getNextDest(core_id_t final_dest, Direction& direction);

   public:
      NetworkModelEMeshHopByHop(Network* net); : NetworkModel(net) { }
      ~NetworkModelEMeshHopByHop();

      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);

      void outputSummary(std::ostream &out);
      void enable();
      void disable();
      bool isEnabled();
};

#endif /* __NETWORK_MODEL_EMESH_HOP_BY_HOP_H__ */
