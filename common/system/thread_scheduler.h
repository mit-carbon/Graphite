#ifndef THREAD_SCHEDULER_SERVER_H
#define THREAD_SCHEDULER_SERVER_H

#include <vector>
#include <queue>
#include <map>

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

   virtual void yieldThread();
   virtual void masterYieldThread(ThreadYieldRequest* req);

protected:

   friend class LCP;
   friend class MCP;

   bool m_master;
   bool m_enable_pep_cores;

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
   UInt32 m_thread_switch_time;
};

#endif // THREAD_SCHEDULER_SERVER_H
