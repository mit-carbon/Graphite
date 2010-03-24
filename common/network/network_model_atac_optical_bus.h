#ifndef __NETWORK_MODEL_ATAC_OPTICAL_BUS_H__
#define __NETWORK_MODEL_ATAC_OPTICAL_BUS_H__

#include "queue_model.h"
#include "network.h"
#include "lock.h"

// Single Sender Multiple Receivers Model
// 1 sender, N receivers (1 to N)
class NetworkModelAtacOpticalBus : public NetworkModel
{
   private:
      UInt64 m_optical_latency;
      UInt32 m_optical_bandwidth;
      UInt32 m_num_application_cores;
      UInt32 m_total_cores;
 
      QueueModel* m_ejection_port_queue_model;

      bool m_queue_model_enabled;
      bool m_enabled;

      // Lock
      Lock m_lock;

      // Performance Counters
      UInt64 m_total_bytes_received;
      UInt64 m_total_packets_received;
      UInt64 m_total_contention_delay;
      UInt64 m_total_packet_latency;

      UInt64 computeLatency(UInt64 pkt_time, UInt32 pkt_length, core_id_t requester);
      UInt64 computeEjectionPortQueueDelay(UInt64 pkt_time, UInt32 pkt_length);
      core_id_t getRequester(const NetPacket& pkt);
      UInt64 computeProcessingTime(UInt32 pkt_length);

   public:
      NetworkModelAtacOpticalBus(Network *net);
      ~NetworkModelAtacOpticalBus();

      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket& pkt);
      
      void enable();
      void disable();
      
      void outputSummary(std::ostream &out);
};

#endif /* __NETWORK_MODEL_ATAC_OPTICAL_BUS_H__ */
