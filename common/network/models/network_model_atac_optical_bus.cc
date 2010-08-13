#include <cmath>

#include "network_model_atac_optical_bus.h"
#include "core.h"
#include "config.h"
#include "log.h"
#include "simulator.h"
#include "memory_manager_base.h"
#include "packet_type.h"
#include "clock_converter.h"

NetworkModelAtacOpticalBus::NetworkModelAtacOpticalBus(Network *net, SInt32 network_id):
   NetworkModel(net, network_id),
   m_enabled(false)
{
   // The frequency of the network is assumed to be the frequency at which the optical links are operated
   m_total_cores = Config::getSingleton()->getTotalCores();
  
   volatile double waveguide_length; 
   try
   {
      m_optical_network_frequency = Sim()->getCfg()->getFloat("network/atac_optical_bus/onet/frequency");
      m_optical_network_link_width = Sim()->getCfg()->getInt("network/atac_optical_bus/onet/link/width");
      waveguide_length = Sim()->getCfg()->getFloat("network/atac_optical_bus/onet/link/length");
      
      m_queue_model_enabled = Sim()->getCfg()->getBool("network/atac_optical_bus/queue_model/enabled");
      m_queue_model_type = Sim()->getCfg()->getString("network/atac_optical_bus/queue_model/type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading atac_optical_bus network model parameters");
   }

   // Create the Optical Network Link Model
   m_optical_network_link_model = new OpticalNetworkLinkModel(m_optical_network_frequency, \
         waveguide_length, \
         m_optical_network_link_width);

   m_optical_network_link_delay = m_optical_network_link_model->getDelay();

   // Create the Ejection Port Contention Models
   if (m_queue_model_enabled)
   {
      UInt64 min_processing_time = 1;
      m_ejection_port_queue_model = QueueModel::create(m_queue_model_type, min_processing_time);
   }

   // Initialize Performance Counters
   initializePerformanceCounters();

   initializeActivityCounters();
}

NetworkModelAtacOpticalBus::~NetworkModelAtacOpticalBus()
{
   if (m_queue_model_enabled)
      delete m_ejection_port_queue_model;
   
   // Delete the Optical Link Model
   delete m_optical_network_link_model;
}

void
NetworkModelAtacOpticalBus::initializePerformanceCounters()
{
   m_total_bytes_received = 0;
   m_total_packets_received = 0;
   m_total_contention_delay = 0;
   m_total_packet_latency = 0;
}

void
NetworkModelAtacOpticalBus::initializeActivityCounters()
{
   m_optical_network_link_traversals = 0;
}

core_id_t
NetworkModelAtacOpticalBus::getRequester(const NetPacket& pkt)
{
   core_id_t requester = INVALID_CORE_ID;

   if ((pkt.type == SHARED_MEM_1) || (pkt.type == SHARED_MEM_2))
      requester = getNetwork()->getCore()->getMemoryManager()->getShmemRequester(pkt.data);
   else // Other Packet types
      requester = pkt.sender;

   LOG_ASSERT_ERROR((requester >= 0) && (requester < (core_id_t) Config::getSingleton()->getTotalCores()), \
         "requester(%i)", requester);
   
   return requester;
}

UInt64 
NetworkModelAtacOpticalBus::computeProcessingTime(UInt32 pkt_length, volatile double bandwidth)
{
   // Send: (pkt_length * 8) bits
   // Bandwidth: (bandwidth) bits/cycle
   // We need floating point comparison here since we are dividing frequencies
   return (UInt64) (ceil(((double) (pkt_length * 8)) / bandwidth));
}

UInt64
NetworkModelAtacOpticalBus::computeEjectionPortQueueDelay(UInt64 pkt_time, UInt32 pkt_length, core_id_t requester)
{
   if ((!m_enabled) || (!m_queue_model_enabled) || (requester >= (core_id_t) Config::getSingleton()->getApplicationCores()))
      return 0;

   // Convert pkt_time from global clock to optical clock
   UInt64 processing_time = computeProcessingTime(pkt_length, (volatile double) m_optical_network_link_width);
   return m_ejection_port_queue_model->computeQueueDelay(pkt_time, processing_time);
}

UInt32
NetworkModelAtacOpticalBus::computeAction(const NetPacket& pkt)
{
   core_id_t core_id = getNetwork()->getCore()->getId();
   LOG_ASSERT_ERROR((pkt.receiver == NetPacket::BROADCAST) || (pkt.receiver == core_id),
         "pkt.receiver(%i), core_id(%i)", pkt.receiver, core_id);

   return RoutingAction::RECEIVE;
}

void 
NetworkModelAtacOpticalBus::routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops)
{
   ScopedLock sl(m_lock);

   if (pkt.receiver == NetPacket::BROADCAST)
   {
      // Update Dynamic Energy here - We use the optical link here once for broadcast
      updateDynamicEnergy(pkt);

      for (core_id_t i = 0; i < (core_id_t) m_total_cores; i++)
      {
         Hop h;
         h.next_dest = i;
         h.final_dest = NetPacket::BROADCAST;
         if (getNetwork()->getCore()->getId() != i)
         {
            h.time = pkt.time + m_optical_network_link_delay;
         }
         else
         {
            h.time = pkt.time;
         }
         nextHops.push_back(h);
      }
   }
   else
   {
      // This is a multicast or unicast
      // Right now, it is a unicast
      // Add support for multicast later
      LOG_ASSERT_ERROR(pkt.receiver < (core_id_t) m_total_cores,
            "Got invalid receiver ID = %i", pkt.receiver);

      Hop h;
      h.next_dest = pkt.receiver;
      h.final_dest = pkt.receiver;
      if (getNetwork()->getCore()->getId() != pkt.receiver)
      {
         // Update Dynamic Energy here - We use the optical link here for unicast
         updateDynamicEnergy(pkt);

         h.time = pkt.time + m_optical_network_link_delay;
      }
      else
      {
         h.time = pkt.time;
      }
      nextHops.push_back(h);
   }
}

void
NetworkModelAtacOpticalBus::processReceivedPacket(NetPacket& pkt)
{
   ScopedLock sl(m_lock);

   core_id_t requester = getRequester(pkt);
   if ((!m_enabled) || (requester >= (core_id_t) Config::getSingleton()->getApplicationCores()))
      return;

   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
  
   UInt64 packet_latency = pkt.time - pkt.start_time;
   UInt64 contention_delay = 0;

   if (pkt.sender != getNetwork()->getCore()->getId())
   {
      UInt64 processing_time = computeProcessingTime(pkt_length, (volatile double) m_optical_network_link_width);
      UInt64 ejection_port_queue_delay = computeEjectionPortQueueDelay(pkt.time, pkt_length, requester);

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
      UInt64 total_contention_delay_in_ns = convertCycleCount(m_total_contention_delay, getFrequency(), 1.0);
      UInt64 total_packet_latency_in_ns = convertCycleCount(m_total_packet_latency, getFrequency(), 1.0);

      out << "    average contention delay (in clock cycles): " << 
         ((float) m_total_contention_delay / m_total_packets_received) << endl;
      out << "    average contention delay (in ns): " << 
         ((float) total_contention_delay_in_ns / m_total_packets_received) << endl;
      
      out << "    average packet latency (in clock cycles): " <<
         ((float) m_total_packet_latency / m_total_packets_received) << endl;
      out << "    average packet latency (in ns): " <<
         ((float) total_packet_latency_in_ns / m_total_packets_received) << endl;
   }
   else
   {
      out << "    average contention delay (in clock cycles): 0" << endl;
      out << "    average contention delay (in ns): 0" << endl;
      
      out << "    average packet latency (in clock cycles): 0" << endl;
      out << "    average packet latency (in ns): 0" << endl;
   }

   outputPowerSummary(out);
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

// Power/Energy related functions
void
NetworkModelAtacOpticalBus::updateDynamicEnergy(const NetPacket& pkt)
{
   core_id_t requester = getRequester(pkt);
   if ((!m_enabled) || (requester >= (core_id_t) Config::getSingleton()->getApplicationCores()))
      return;

   // FIXME: Assume half the bits flip for now
   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
   UInt32 num_flits = computeProcessingTime(pkt_length, (volatile double) m_optical_network_link_width);

   if (Config::getSingleton()->getEnablePowerModeling())
   {
      m_optical_network_link_model->updateDynamicEnergy(m_optical_network_link_width/2, num_flits);
   }
   m_optical_network_link_traversals ++;
}

void
NetworkModelAtacOpticalBus::outputPowerSummary(ostream& out)
{
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      volatile double static_power = m_optical_network_link_model->getStaticPower();
      volatile double dynamic_energy = m_optical_network_link_model->getDynamicEnergy();

      out << "    Static Power: " << static_power << endl;
      out << "    Dynamic Energy: " << dynamic_energy << endl;
   }

   out << "  Activity Counters:" << endl;
   out << "    Optical Network Link Traversals: " << m_optical_network_link_traversals << endl; 
}
