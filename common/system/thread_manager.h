#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <vector>
#include <queue>
#include <map>

#include "semaphore.h"
#include "core.h"
#include "fixed_types.h"
#include "message_types.h"
#include "lock.h"

#include "thread_support.h"

class TileManager;
class ThreadScheduler;

class ThreadManager
{
public:
   struct ThreadState
   {
      Core::State status;
      //SInt32 waiter;
      core_id_t waiter_core;
      thread_id_t waiter_tid;
      thread_id_t thread_id;

      ThreadState()
         : status(Core::IDLE)
         , waiter_core(INVALID_CORE_ID)
         , waiter_tid(INVALID_THREAD_ID)
      {} 
   };

   ThreadManager(TileManager*);
   ~ThreadManager();

   // services
   SInt32 spawnThread(tile_id_t tile_id, thread_func_t func, void *arg);
   void joinThread(thread_id_t thread_id = 0);

   void getThreadToSpawn(ThreadSpawnRequest *req);
   ThreadSpawnRequest* getThreadSpawnReq();
   void dequeueThreadSpawnReq (ThreadSpawnRequest *req);

   void terminateThreadSpawners ();

   // events
   void onThreadStart(ThreadSpawnRequest *req);
   void onThreadExit();


   // misc
   void stallThread(tile_id_t tile_id, thread_id_t thread_id);
   void stallThread(core_id_t core_id, thread_id_t thread_id);
   void stallThread(core_id_t core_id);

   void resumeThread(tile_id_t tile_id, thread_id_t thread_id);
   void resumeThread(core_id_t core_id, thread_id_t thread_id);
   void resumeThread(core_id_t core_id);

   bool isThreadRunning(tile_id_t core_id, thread_id_t thread_id);
   bool isThreadRunning(core_id_t core_id, thread_id_t thread_id);

   bool isThreadInitializing(tile_id_t tile_id, thread_id_t thread_id);
   bool isThreadInitializing(core_id_t core_id, thread_id_t thread_id);
   
   bool areAllCoresRunning();
   thread_id_t isCoreRunning(core_id_t core_id);
   thread_id_t isCoreRunning(tile_id_t tile_id);

   bool isCoreInitializing(tile_id_t tile_id);
   bool isCoreInitializing(core_id_t core_id);

   std::vector< std::vector<ThreadState> > getThreadState() {assert(m_master); return m_thread_state;}

   void setThreadState(tile_id_t tile_id, thread_id_t tidx, Core::State state) {m_thread_state[tile_id][tidx].status = state;}

   friend class ThreadScheduler;
   void setThreadScheduler(ThreadScheduler* thread_scheduler) {m_thread_scheduler = thread_scheduler;}

private:

   friend class LCP;
   friend class MCP;

   void masterSpawnThread(ThreadSpawnRequest*);
   void slaveSpawnThread(ThreadSpawnRequest*);
   void masterSpawnThreadReply(ThreadSpawnRequest*);


   void masterOnThreadStart(tile_id_t tile_id, UInt32 core_type, SInt32 thread_idx);
   void masterOnThreadExit(tile_id_t tile_id, UInt32 core_type, SInt32 thread_idx, UInt64 time);

   void slaveTerminateThreadSpawnerAck (tile_id_t);
   void slaveTerminateThreadSpawner ();
   void updateTerminateThreadSpawner ();

   void masterJoinThread(ThreadJoinRequest *req, UInt64 time);
   void wakeUpWaiter(core_id_t core_id, thread_id_t thread_id, UInt64 time);
   void wakeUpMainWaiter(core_id_t core_id, thread_id_t thread_id, UInt64 time);

   void insertThreadSpawnRequest (ThreadSpawnRequest *req);

   thread_id_t getNewThreadId(core_id_t core_id, thread_id_t thread_index);


   thread_id_t m_tid_counter;
   Lock m_tid_counter_lock;

   bool m_master;
   std::vector< std::vector<ThreadState> > m_thread_state;

   std::vector<thread_id_t> m_last_stalled_thread;

   std::vector< std::pair<core_id_t, thread_id_t> > m_tid_to_core_map;

   std::queue<ThreadSpawnRequest*> m_thread_spawn_list;
   Semaphore m_thread_spawn_sem;
   Lock m_thread_spawn_lock;


   TileManager *m_tile_manager;
   ThreadScheduler *m_thread_scheduler;

   Lock m_thread_spawners_terminated_lock;
   UInt32 m_thread_spawners_terminated;
};

#endif // THREAD_MANAGER_H
