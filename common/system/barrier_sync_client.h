#ifndef __BARRIER_SYNC_CLIENT_H__
#define __BARRIER_SYNC_CLIENT_H__

#include <cassert>

#include "clock_skew_minimization_object.h"
#include "fixed_types.h"
#include "packetize.h"

// Forward Decls
class Core;

class BarrierSyncClient : public ClockSkewMinimizationClient
{
   private:
      Core* m_core;

      UInt64 m_barrier_interval;
      UInt64 m_next_sync_time;

   public:
      BarrierSyncClient(Core* core);
      ~BarrierSyncClient();

      void synchronize(void);
      void netProcessSyncMsg(NetPacket& packet) { assert(false); }

      static const unsigned int BARRIER_RELEASE = 0xBABECAFE;
};

#endif /* __BARRIER_SYNC_CLIENT_H__ */
