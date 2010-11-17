#ifndef __RING_SYNC_CLIENT_H__
#define __RING_SYNC_CLIENT_H__

#include <cassert>

#include "clock_skew_minimization_object.h"
#include "core.h"
#include "tile.h"
#include "lock.h"
#include "cond.h"
#include "fixed_types.h"

class RingSyncClient : public ClockSkewMinimizationClient
{
   private:
      Core* _core;

      UInt64 _cycle_count;
      UInt64 _max_cycle_count;

      Lock _lock;
      ConditionVariable _cond;

   public:
      RingSyncClient(Core* core);
      ~RingSyncClient();

      void enable() {}
      void disable() {}

      void synchronize(UInt64 time);
      void netProcessSyncMsg(const NetPacket& packet) { assert(false); }

      Lock* getLock() { return &_lock; }
      UInt64 getCycleCount() { return _cycle_count; }
      void setMaxCycleCount(UInt64 max_cycle_count);
};

#endif /* __RING_SYNC_CLIENT_H__ */
