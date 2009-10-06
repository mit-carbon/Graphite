#ifndef __SIMULATION_BARRIER_CLIENT_H__
#define __SIMULATION_BARRIER_CLIENT_H__

#include "fixed_types.h"
#include "packetize.h"

// Forward Decls
class Core;

class SimulationBarrierClient
{
   private:
      Core* m_core;
      UnstructuredBuffer m_send_buff;
      UnstructuredBuffer m_recv_buff;

      UInt64 m_barrier_interval;
      UInt64 m_next_sync_time;

   public:
      SimulationBarrierClient(Core* core);
      ~SimulationBarrierClient();

      void handlePeriodicBarrier();

      static const unsigned int SIM_BARRIER_RELEASE = 0xBABECAFE;
};

#endif /* __SIMULATION_BARRIER_CLIENT_H__ */
