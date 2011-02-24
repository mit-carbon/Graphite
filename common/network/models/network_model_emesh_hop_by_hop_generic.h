#ifndef __NETWORK_MODEL_EMESH_HOP_BY_HOP_GENERIC_H__
#define __NETWORK_MODEL_EMESH_HOP_BY_HOP_GENERIC_H__

#include "network.h"
#include "network_model.h"
#include "fixed_types.h"
#include "queue_model.h"
#include "lock.h"
#include "electrical_network_router_model.h"
#include "electrical_network_link_model.h"

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
      tile_id_t m_tile_id;
      static SInt32 m_mesh_width;
      static SInt32 m_mesh_height;

      // Router & Link Parameters
      UInt32 m_num_router_ports;
      UInt64 m_hop_latency;

      ElectricalNetworkRouterModel* m_electrical_router_model;
      ElectricalNetworkLinkModel* m_electrical_link_model;

      QueueModel* m_queue_models[NUM_OUTPUT_DIRECTIONS];
      QueueModel* m_injection_port_queue_model;
      QueueModel* m_ejection_port_queue_model;

      bool m_enabled;

      // Lock
      Lock m_lock;

      // Performance Counters
      UInt64 m_total_bytes_received;
      UInt64 m_total_packets_received;
      UInt64 m_total_contention_delay;
      UInt64 m_total_packet_latency;

      // Activity Counters
      UInt64 m_switch_allocator_traversals;
      UInt64 m_crossbar_traversals;
      UInt64 m_buffer_accesses;
      UInt64 m_link_traversals;

      // Functions
      void computePosition(tile_id_t tile, SInt32 &x, SInt32 &y);
      tile_id_t computeTileId(SInt32 x, SInt32 y);
      SInt32 computeDistance(tile_id_t sender, tile_id_t receiver);

      void addHop(OutputDirection direction, tile_id_t final_dest, tile_id_t next_dest, \
            const NetPacket& pkt, UInt64 pkt_time, UInt32 pkt_length, \
            std::vector<Hop>& nextHops, \
            tile_id_t requester);
      UInt64 computeLatency(OutputDirection direction, \
            const NetPacket& pkt, UInt64 pkt_time, UInt32 pkt_length, \
            tile_id_t requester);
      UInt64 computeProcessingTime(UInt32 pkt_length);
      tile_id_t getNextDest(tile_id_t final_dest, OutputDirection& direction);
      tile_id_t getRequester(const NetPacket& pkt);

      // Injection & Ejection Port Queue Models
      UInt64 computeInjectionPortQueueDelay(tile_id_t pkt_receiver, UInt64 pkt_time, UInt32 pkt_length);
      UInt64 computeEjectionPortQueueDelay(const NetPacket& pkt, UInt64 pkt_time, UInt32 pkt_length);

      static void initializeEMeshTopologyParams();
      void createQueueModels();
      void destroyQueueModels();
      void resetQueueModels();

      // Router & Link Models
      void createRouterAndLinkModels();
      void destroyRouterAndLinkModels();

      // Performance Counters
      void initializePerformanceCounters();
      // Activity Counters for Power
      void initializeActivityCounters();
      
      // Update Dynamic Energy
      void updateDynamicEnergy(const NetPacket& pkt, bool is_buffered, UInt32 contention);
      void outputPowerSummary(std::ostream& out);

   protected:
      volatile float m_frequency;

      // Link Characteristics
      UInt32 m_link_width;
      volatile double m_link_length;
      std::string m_link_type;

      // Router Characteristics
      UInt64 m_router_delay;
      UInt32 m_num_flits_per_output_buffer;

      // Check if the electrical mesh has a broadcast tree
      bool m_broadcast_tree_enabled;
      
      bool m_queue_model_enabled;
      std::string m_queue_model_type;

      void initializeModels();
   
   public:
      NetworkModelEMeshHopByHopGeneric(Network* net, SInt32 network_id);
      ~NetworkModelEMeshHopByHopGeneric();

      volatile float getFrequency() { return m_frequency; }

      UInt32 computeAction(const NetPacket& pkt);
      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket &pkt);

      static std::pair<bool,std::vector<tile_id_t> > computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 tile_count);
      static std::pair<bool,SInt32> computeTileCountConstraints(SInt32 tile_count);
      static std::pair<bool,std::vector<Config::TileList> > computeProcessToTileMapping();

      static SInt32 computeNumHops(tile_id_t sender, tile_id_t receiver);

      void outputSummary(std::ostream &out);

      void enable();
      void disable();
      void reset();
};

#endif /* __NETWORK_MODEL_EMESH_HOP_BY_HOP_GENERIC_H__ */
