#ifndef __NETWORK_MODEL_EMESH_HOP_BY_HOP_GENERIC_H__
#define __NETWORK_MODEL_EMESH_HOP_BY_HOP_GENERIC_H__

#include "network.h"
#include "network_model.h"
#include "fixed_types.h"
#include "queue_model.h"
#include "lock.h"

class NetworkModelEMeshHopByHopGeneric : public NetworkModel
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
      core_id_t m_core_id;
      SInt32 m_mesh_width;
      SInt32 m_mesh_height;

      QueueModel* m_queue_models[NUM_OUTPUT_DIRECTIONS];

      bool m_enabled;

      // Lock
      Lock m_lock;

      // Counters
      UInt64 m_num_bytes;
      UInt64 m_num_packets;
      UInt64 m_total_contention_delay;
      UInt64 m_total_latency;

      // Functions
      void computePosition(core_id_t core, SInt32 &x, SInt32 &y);
      core_id_t computeCoreId(SInt32 x, SInt32 y);
      SInt32 computeDistance(core_id_t sender, core_id_t receiver);

      void addHop(OutputDirection direction, core_id_t final_dest, core_id_t next_dest, UInt64 pkt_time, UInt32 pkt_length, std::vector<Hop>& nextHops, core_id_t requester);
      UInt64 computeLatency(OutputDirection direction, UInt64 pkt_time, UInt32 pkt_length, core_id_t requester);
      UInt64 computeSerializationLatency(UInt32 pkt_length);
      UInt64 computeProcessingTime(UInt32 pkt_length);
      core_id_t getNextDest(core_id_t final_dest, OutputDirection& direction);

   protected:
      UInt32 m_link_bandwidth;
      UInt64 m_hop_latency;
      bool m_broadcast_tree_enabled;
      
      bool m_queue_model_enabled;
      std::string m_queue_model_type;

      void createQueueModels();
   
   public:
      NetworkModelEMeshHopByHopGeneric(Network* net);
      ~NetworkModelEMeshHopByHopGeneric();

      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket &pkt);
      static std::pair<bool,std::vector<core_id_t> > computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 core_count);
      static std::pair<bool,SInt32> computeCoreCountConstraints(SInt32 core_count);
      static SInt32 computeNumHops(core_id_t sender, core_id_t receiver);

      void outputSummary(std::ostream &out);
      void enable();
      void disable();
      bool isEnabled();
};

#endif /* __NETWORK_MODEL_EMESH_HOP_BY_HOP_GENERIC_H__ */
