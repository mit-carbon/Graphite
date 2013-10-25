#pragma once

#include <cassert>

#include "clock_skew_management_object.h"
#include "fixed_types.h"
#include "packetize.h"
#include "time_types.h"

// Forward Decls
class Core;

class LaxBarrierSyncClient : public ClockSkewManagementClient
{
private:
   Core* m_core;

   UInt64 m_barrier_interval;
   UInt64 m_next_sync_time;

public:
   LaxBarrierSyncClient(Core* core);
   ~LaxBarrierSyncClient();

   void enable() {}
   void disable() {}

   void synchronize(Time time);
   void netProcessSyncMsg(const NetPacket& packet) { assert(false); }

   static const unsigned int BARRIER_RELEASE = 0xBABECAFE;
};
