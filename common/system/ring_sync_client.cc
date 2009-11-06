#include "ring_sync_client.h"

RingSyncClient::RingSyncClient(Core* core):
   _core(core),
   _cycle_count(0),
   _max_cycle_count(0)
{}

RingSyncClient::~RingSyncClient()
{}

// Called by the core thread
void
RingSyncClient::synchronize()
{
   _lock.acquire();
   _cycle_count = _core->getPerformanceModel()->getCycleCount();

   while (_cycle_count >= _max_cycle_count)
   {
      _cond.wait(_lock);
   }
   _lock.release();
}

// Called by the LCP
void
RingSyncClient::setMaxCycleCount(UInt64 max_cycle_count)
{
   assert (max_cycle_count >= _max_cycle_count);
   _max_cycle_count = max_cycle_count;
   
   if (_cycle_count < _max_cycle_count)
   {
      _cond.signal();
   }
}
