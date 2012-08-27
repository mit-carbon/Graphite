#ifndef THREAD_SCHEDULER_SERVER_H
#define THREAD_SCHEDULER_SERVER_H

#include <vector>
#include <bitset>
#include <queue>
#include <map>
//#include <sched.h>

#include "cond.h"
#include "core.h"
#include "fixed_types.h"
#include "message_types.h"
#include "lock.h"

#include "thread_support.h"

class ThreadManager;
class TileManager;

class ThreadScheduler
{
protected:
   ThreadScheduler(ThreadManager*, TileManager*);

public:
   ~ThreadScheduler();
   static ThreadScheduler* create(ThreadManager*, TileManager*);

   void masterScheduleThread(ThreadSpawnRequest *req);
   void masterStartThread(core_id_t core_id);

   void onThreadExit();
   void masterOnThreadExit(core_id_t core_id, SInt32 thread_idx);

   void disablePreemptiveScheduling(){m_thread_preemption_enabled = false; m_thread_migration_enabled = false;}
   void enablePreemptiveScheduling(){m_thread_preemption_enabled = true; m_thread_migration_enabled = true;}

   void migrateThread(thread_id_t thread_id, tile_id_t tile_id);
   void masterMigrateThread(thread_id_t src_thread_id, tile_id_t dst_tile_id, UInt32 dst_core_type);
   void masterMigrateThread(thread_id_t src_thread_idx, core_id_t src_core_id, thread_id_t dst_thread_idx, core_id_t dst_core_id);

   bool schedSetAffinity(thread_id_t tid, unsigned int cpusetsize, cpu_set_t* set);
   void masterSchedSetAffinity(ThreadAffinityRequest * req);
   bool schedGetAffinity(thread_id_t tid, unsigned int cpusetsize, cpu_set_t* set);
   void masterSchedGetAffinity(ThreadAffinityRequest * req);

   void yieldThread(bool is_pre_emptive = true);
   void masterYieldThread(ThreadYieldRequest* req);

   // Implement these functions for different scheduling types.
   virtual void enqueueThread(core_id_t core_id, ThreadSpawnRequest * req);
   virtual void requeueThread(core_id_t core_id);

   thread_id_t getNextThreadIdx(core_id_t core_id);

protected:

   friend class LCP;
   friend class MCP;

   bool masterCheckAffinityAndMigrate(core_id_t core_id, thread_id_t thread_idx, core_id_t &dst_core_id, thread_id_t &dst_thread_idx);
   bool m_master;

   UInt32 m_total_tiles;
   UInt32 m_threads_per_core;

   TileManager * m_tile_manager;
   ThreadManager * m_thread_manager;

   // Used to wake up proper thread on the client side.
   std::vector<thread_id_t> m_local_next_tidx;

   std::vector<Lock> m_core_lock;
   std::vector<ConditionVariable> m_thread_wait_cond;

   std::vector< std::queue<ThreadSpawnRequest*> > m_waiter_queue;

   std::vector< std::vector<UInt32> > m_last_start_time;

   bool m_thread_migration_enabled;
   bool m_thread_preemption_enabled;
   UInt32 m_thread_switch_quantum;
   bool m_enabled;
};

#endif // THREAD_SCHEDULER_SERVER_H
