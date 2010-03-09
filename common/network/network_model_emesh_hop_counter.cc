#include <stdlib.h>
#include <math.h>

#include "simulator.h"
#include "config.h"
#include "network_model_emesh_hop_counter.h"
#include "config.h"

NetworkModelEMeshHopCounter::NetworkModelEMeshHopCounter(Network *net)
   : NetworkModel(net)
   , _enabled(false)
   , _bytesSent(0)
   , _cyclesLatency(0)
{
   SInt32 total_cores = Config::getSingleton()->getTotalCores();

   _meshWidth = (SInt32) floor (sqrt(total_cores));
   _meshHeight = (SInt32) ceil (1.0 * total_cores / _meshWidth);

   try
   {
      _linkBandwidth = Sim()->getCfg()->getInt("network/emesh_hop_counter/link_bandwidth");
      _hopLatency = (UInt64) Sim()->getCfg()->getInt("network/emesh_hop_counter/hop_latency");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read emesh_hop_counter paramters from the cfg file");
   }

   assert(total_cores <= _meshWidth * _meshHeight);
   assert(total_cores > (_meshWidth - 1) * _meshHeight);
   assert(total_cores > _meshWidth * (_meshHeight - 1));
}

NetworkModelEMeshHopCounter::~NetworkModelEMeshHopCounter()
{
}

void NetworkModelEMeshHopCounter::computePosition(core_id_t core,
                                             SInt32 &x, SInt32 &y)
{
   x = core % _meshWidth;
   y = core / _meshWidth;
}

SInt32 NetworkModelEMeshHopCounter::computeDistance(SInt32 x1, SInt32 y1, SInt32 x2, SInt32 y2)
{
   return abs(x1 - x2) + abs(y1 - y2);
}

void NetworkModelEMeshHopCounter::routePacket(const NetPacket &pkt,
                                         std::vector<Hop> &nextHops)
{
   UInt64 curr_time = pkt.time;
   UInt64 serialization_latency = computeSerializationLatency(pkt.length);

   SInt32 sx, sy, dx, dy;

   computePosition(pkt.sender, sx, sy);

   if (pkt.receiver == NetPacket::BROADCAST)
   {
      UInt32 total_cores = Config::getSingleton()->getTotalCores();
      UInt64 total_latency = 0;
      // There's no broadcast tree here, but I guess that won't be a
      // bottleneck at all since there's no contention
      for (SInt32 i = 0; i < (SInt32) total_cores; i++)
      {
         computePosition(i, dx, dy);

         UInt64 latency = computeDistance(sx, sy, dx, dy) * _hopLatency;
         if (i != pkt.sender)
            latency += serialization_latency;

         total_latency += latency;

         Hop h;
         h.final_dest = i;
         h.next_dest = i;
         h.time = curr_time + latency;

         // Update curr_time
         curr_time += serialization_latency;

         nextHops.push_back(h);
      }
      
      if (!_enabled)
         return;

      _lock.acquire();
      _bytesSent += total_cores * pkt.length;
      _cyclesLatency += total_latency;
      _lock.release();
   } 
   else
   {
      computePosition(pkt.receiver, dx, dy);

      UInt64 latency = computeDistance(sx, sy, dx, dy) * _hopLatency;
      if (pkt.receiver != pkt.sender)
         latency += serialization_latency;

      Hop h;
      h.final_dest = pkt.receiver;
      h.next_dest = pkt.receiver;
      h.time = pkt.time + latency;

      nextHops.push_back(h);

      if (!_enabled)
         return;

      _lock.acquire();
      _bytesSent += pkt.length;
      _cyclesLatency += latency;
      _lock.release();
   }
}

UInt64 
NetworkModelEMeshHopCounter::computeSerializationLatency(UInt32 pkt_length)
{
   // Send: (pkt_length * 8) bits
   // Bandwidth: (m_link_bandwidth) bits/cycle
   UInt32 num_bits = pkt_length * 8;
   if (num_bits % _linkBandwidth == 0)
      return (UInt64) (num_bits/_linkBandwidth);
   else
      return (UInt64) (num_bits/_linkBandwidth + 1);
}

void NetworkModelEMeshHopCounter::outputSummary(std::ostream &out)
{
   out << "    bytes sent: " << _bytesSent << std::endl;
   out << "    cycles spent latency: " << _cyclesLatency << std::endl;
}
