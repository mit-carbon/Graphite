#ifndef __NETWORK_MODEL_ATAC_OPTICAL_BUS_H__
#define __NETWORK_MODEL_ATAC_OPTICAL_BUS_H__

#include "queue_model.h"
#include "network.h"
#include "lock.h"
#include "optical_network_link_model.h"

// Single Sender Multiple Receivers Model
// 1 sender, N receivers (1 to N)
class NetworkModelAtacOpticalBus : public NetworkModel
{
   private:
      UInt32 m_total_cores;

      // Optical Network Link Parameters
      volatile float m_optical_network_frequency;
      UInt32 m_optical_network_link_width;
      UInt64 m_optical_network_link_delay;

      OpticalNetworkLinkModel* m_optical_network_link_model;

      QueueModel* m_ejection_port_queue_model;

      bool m_queue_model_enabled;
      std::string m_queue_model_type;
      bool m_enabled;

      // Lock
      Lock m_lock;

      // Performance Counters
      UInt64 m_total_bytes_received;
      UInt64 m_total_packets_received;
      UInt64 m_total_contention_delay;
      UInt64 m_total_packet_latency;

      core_id_t getRequester(const NetPacket& pkt);
      UInt64 computeProcessingTime(UInt32 pkt_length, volatile double bandwidth);
      // Here we assume that even if N flits can arrive at a receiver at a given time,
      // only 1 flit can be accepted by the receiver
      UInt64 computeEjectionPortQueueDelay(UInt64 pkt_time, UInt32 pkt_length, core_id_t requester);

      void initializePerformanceCounters();

      // Energy/Power related Functions
      void updateDynamicEnergy(const NetPacket& pkt);
      void outputPowerSummary(std::ostream& out);

   public:
      NetworkModelAtacOpticalBus(Network *net, SInt32 network_id);
      ~NetworkModelAtacOpticalBus();

      volatile float getFrequency() { return m_optical_network_frequency; }

      UInt32 computeAction(const NetPacket& pkt);
      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket& pkt);

      void enable();
      void disable();
      
      void outputSummary(std::ostream &out);
};

#endif /* __NETWORK_MODEL_ATAC_OPTICAL_BUS_H__ */
