#include <stdlib.h>
#include <math.h>

#include "network_model_hop_counter.h"
#include "config.h"

NetworkModelHopCounter::NetworkModelHopCounter(Network *net)
   : NetworkModel(net)
   , _enabled(true)
   , _bytesSent(0)
   , _cyclesLatency(0)
   , _hopLatency(1)
{
   SInt32 total_cores = Config::getSingleton()->getTotalCores();

   _meshWidth = (SInt32) floor (sqrt(total_cores));
   _meshHeight = (SInt32) ceil (1.0 * total_cores / _meshWidth);

   assert(total_cores <= _meshWidth * _meshHeight);
   assert(total_cores > (_meshWidth - 1) * _meshHeight);
   assert(total_cores > _meshWidth * (_meshHeight - 1));
}

NetworkModelHopCounter::~NetworkModelHopCounter()
{
}

void NetworkModelHopCounter::computePosition(core_id_t core,
                                             SInt32 &x, SInt32 &y)
{
   x = core % _meshWidth;
   y = core / _meshWidth;
}

SInt32 NetworkModelHopCounter::computeDistance(SInt32 x1, SInt32 y1, SInt32 x2, SInt32 y2)
{
   return abs(x1 - x2) + abs(y1 - y2);
}

void NetworkModelHopCounter::routePacket(const NetPacket &pkt,
                                         std::vector<Hop> &nextHops)
{
   SInt32 sx, sy, dx, dy;

   computePosition(pkt.sender, sx, sy);

   if (pkt.receiver == NetPacket::BROADCAST)
   {
      UInt32 total_cores = Config::getSingleton()->getTotalCores();
      UInt64 total_latency = 0;
      for (SInt32 i = 0; i < (SInt32) total_cores; i++)
      {
         computePosition(i, dx, dy);

         UInt64 latency = computeDistance(sx, sy, dx, dy) * _hopLatency;
         total_latency += latency;

         Hop h;
         h.dest = i;
         h.time = pkt.time + latency;

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

      Hop h;
      h.dest = pkt.receiver;
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

void NetworkModelHopCounter::outputSummary(std::ostream &out)
{
   out << "    bytes sent: " << _bytesSent << std::endl;
   out << "    cycles spent latency: " << _cyclesLatency << std::endl;
}
