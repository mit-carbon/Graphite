#ifndef NETWORK_MODEL_ANALYTICAL_H
#define NETWORK_MODEL_ANALYTICAL_H

#include "network.h"
#include "lock.h"
#include "network_model_analytical_params.h"

class Lock;

class NetworkModelAnalytical : public NetworkModel
{
   public:
      NetworkModelAnalytical(Network *net, SInt32 network_id);
      ~NetworkModelAnalytical();

      volatile float getFrequency() { return _frequency; }

      UInt32 computeAction(const NetPacket& pkt);
      void routePacket(const NetPacket &pkt,
                       std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket& pkt) {}

      void outputSummary(std::ostream &out);

      void reset();
      
   private:
      UInt64 computeLatency(const NetPacket &);
      void updateUtilization();
      static void receiveMCPUpdate(void *, NetPacket);

      volatile float _frequency;

      UInt64 _bytesSent;
      UInt64 _cyclesProc;
      UInt64 _cyclesLatency;
      UInt64 _cyclesContention;

      double _globalUtilization;
      UInt64 _localUtilizationLastUpdate;
      UInt64 _localUtilizationFlitsSent;

      Lock _lock;

      NetworkModelAnalyticalParameters _params;

      void initializePerformanceCounters();
      // Get Flit Width
      UInt32 getFlitWidth() { return _params.W; }
};

#endif // NETWORK_MODEL_ANALYTICAL_H
