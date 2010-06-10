#include "network.h"
#include "network_model_magic.h"
#include "memory_manager_base.h"
#include "log.h"

NetworkModelMagic::NetworkModelMagic(Network *net, SInt32 network_id, float network_frequency) : 
   NetworkModel(net, network_id, network_frequency),
   _enabled(false),
   _num_packets(0),
   _num_bytes(0)
{ }

void
NetworkModelMagic::routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops)
{
   // A latency of '1'
   if (pkt.receiver == NetPacket::BROADCAST)
   {
      UInt32 total_cores = Config::getSingleton()->getTotalCores();
   
      for (SInt32 i = 0; i < (SInt32) total_cores; i++)
      {
         Hop h;
         h.final_dest = i;
         h.next_dest = i;
         h.time = pkt.time + 1;

         nextHops.push_back(h);
      }
   }
   else
   {
      Hop h;
      h.final_dest = pkt.receiver;
      h.next_dest = pkt.receiver;
      h.time = pkt.time + 1;

      nextHops.push_back(h);
   }
}

void
NetworkModelMagic::processReceivedPacket(NetPacket &pkt)
{
   ScopedLock sl(_lock);

   core_id_t requester = INVALID_CORE_ID;

   if ((pkt.type == SHARED_MEM_1) || (pkt.type == SHARED_MEM_2))
      requester = getNetwork()->getCore()->getMemoryManager()->getShmemRequester(pkt.data);
   else // Other Packet types
      requester = pkt.sender;
   
   LOG_ASSERT_ERROR((requester >= 0) && (requester < (core_id_t) Config::getSingleton()->getTotalCores()),
         "requester(%i)", requester);

   if ( (!_enabled) || (requester >= (core_id_t) Config::getSingleton()->getApplicationCores()) )
      return;

   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
   _num_packets ++;
   _num_bytes += pkt_length;
}

void NetworkModelMagic::outputSummary(std::ostream &out)
{
   out << "    num packets received: " << _num_packets << std::endl;
   out << "    num bytes received: " << _num_bytes << std::endl;
}
