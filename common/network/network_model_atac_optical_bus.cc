#include "network_model_atac_optical_bus.h"
#include "core.h"
#include "config.h"
#include "log.h"
#include "simulator.h"
#include "memory_manager_base.h"
#include "packet_type.h"

NetworkModelAtacOpticalBus::NetworkModelAtacOpticalBus(Network *net) :
   NetworkModel(net),
   m_enabled(false),
   m_total_bytes_received(0),
   m_total_packets_received(0),
   m_total_contention_delay(0),
   m_total_packet_latency(0)
{
   m_num_application_cores = Config::getSingleton()->getApplicationCores(); 
   m_total_cores = Config::getSingleton()->getTotalCores();
   
   std::string queue_model_type = ""; 
   try
   {
      // Optical delay is specified in 'clock cycles'
      m_optical_latency = (UInt64) Sim()->getCfg()->getInt("network/atac_optical_bus/latency");
      // Optical Bandwidth is specified in 'bits/clock cycle'
      // In other words, it states the number of wavelengths used
      m_optical_bandwidth = Sim()->getCfg()->getInt("network/atac_optical_bus/bandwidth");
      
      m_queue_model_enabled = Sim()->getCfg()->getBool("network/atac_optical_bus/queue_model/enabled");
      queue_model_type = Sim()->getCfg()->getString("network/atac_optical_bus/queue_model/type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading atac network model parameters");
   }

   if (m_queue_model_enabled)
   {
      UInt64 min_processing_time = 1;
      m_ejection_port_queue_model = QueueModel::create(queue_model_type, min_processing_time);
   }
}

NetworkModelAtacOpticalBus::~NetworkModelAtacOpticalBus()
{
   if (m_queue_model_enabled)
      delete m_ejection_port_queue_model;
}

UInt64
NetworkModelAtacOpticalBus::computeLatency(UInt64 pkt_time, UInt32 pkt_length, core_id_t requester)
{
   if ((!m_enabled) || (requester >= (core_id_t) m_num_application_cores))
      return 0;

   return m_optical_latency;
}

UInt64
NetworkModelAtacOpticalBus::computeEjectionPortQueueDelay(UInt64 pkt_time, UInt32 pkt_length)
{
   if (!m_queue_model_enabled)
      return 0;

   UInt64 processing_time = computeProcessingTime(pkt_length);
   return m_ejection_port_queue_model->computeQueueDelay(pkt_time, processing_time);
}

UInt64 
NetworkModelAtacOpticalBus::computeProcessingTime(UInt32 pkt_length)
{
   // Send: (pkt_length * 8) bits
   // Bandwidth: (m_optical_bandwidth) bits/cycle
   UInt32 num_bits = pkt_length * 8;
   if (num_bits % m_optical_bandwidth == 0)
      return (UInt64) (num_bits/m_optical_bandwidth);
   else
      return (UInt64) (num_bits/m_optical_bandwidth + 1);
}

void 
NetworkModelAtacOpticalBus::routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops)
{
   ScopedLock sl(m_lock);

   core_id_t requester = INVALID_CORE_ID;

   if ((pkt.type == SHARED_MEM_1) || (pkt.type == SHARED_MEM_2))
      requester = getNetwork()->getCore()->getMemoryManager()->getShmemRequester(pkt.data);
   else // Other Packet types
      requester = pkt.sender;

   LOG_ASSERT_ERROR((requester >= 0) && (requester < (core_id_t) Config::getSingleton()->getTotalCores()),
         "requester(%i)", requester);

   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);

   UInt64 net_optical_latency = computeLatency(pkt.time, pkt_length, requester);

   if (pkt.receiver == NetPacket::BROADCAST)
   {
      for (core_id_t i = 0; i < (core_id_t) m_total_cores; i++)
      {
         Hop h;
         h.next_dest = i;
         h.final_dest = i;
         if (getNetwork()->getCore()->getId() != i)
            h.time = pkt.time + net_optical_latency;
         else
            h.time = pkt.time;
         nextHops.push_back(h);
      }
   }
   else
   {
      // This is a multicast or unicast
      // Right now, it is a unicast
      // Add support for multicast later
      LOG_ASSERT_ERROR(pkt.receiver < (core_id_t) m_total_cores, "Got invalid receiver ID = %i", pkt.receiver);

      Hop h;
      h.next_dest = pkt.receiver;
      h.final_dest = pkt.receiver;
      if (getNetwork()->getCore()->getId() != pkt.receiver)
         h.time = pkt.time + net_optical_latency;
      else
         h.time = pkt.time;
      nextHops.push_back(h);
   }
}

void
NetworkModelAtacOpticalBus::processReceivedPacket(NetPacket& pkt)
{
   ScopedLock sl(m_lock);
      
   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
  
   UInt64 packet_latency = pkt.time - pkt.start_time;
   UInt64 contention_delay = 0;

   if (pkt.sender != getNetwork()->getCore()->getId())
   {
      UInt64 processing_time = computeProcessingTime(pkt_length);
      UInt64 ejection_port_queue_delay = computeEjectionPortQueueDelay(pkt.time, pkt_length);

      packet_latency += (ejection_port_queue_delay + processing_time);
      contention_delay += ejection_port_queue_delay;
      pkt.time += (ejection_port_queue_delay + processing_time);
   }

   // Performance Counters
   m_total_packets_received ++;
   m_total_bytes_received += (UInt64) pkt_length;
   m_total_packet_latency += packet_latency;
   m_total_contention_delay += contention_delay;

}

void
NetworkModelAtacOpticalBus::outputSummary(std::ostream &out)
{
   out << "    bytes received: " << m_total_bytes_received << std::endl;
   out << "    packets received: " << m_total_packets_received << std::endl;
   if (m_total_packets_received > 0)
   {
      out << "    average queueing delay: " << 
         ((float) m_total_contention_delay / m_total_packets_received) << std::endl;
      out << "    average packet latency: " <<
         ((float) m_total_packet_latency / m_total_packets_received) << std::endl;
   }
   else
   {
      out << "    average queueing delay: " << 
         "NA" << std::endl;
      out << "    average packet latency: " <<
         "NA" << std::endl;
   }
}

void
NetworkModelAtacOpticalBus::enable()
{
   m_enabled = true;
}

void
NetworkModelAtacOpticalBus::disable()
{
   m_enabled = false;
}
