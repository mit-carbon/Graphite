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

class ThreadManager
{
public:
   ThreadManager(TileManager*);
   ~ThreadManager();

   // services
   SInt32 spawnThread(thread_func_t func, void *arg);
   void joinThread(tile_id_t tile_id);
   
   SInt32 spawnHelperThread(thread_func_t func, void *arg);
   void joinHelperThread(tile_id_t tile_id);

   void getThreadToSpawn(ThreadSpawnRequest *req);
   ThreadSpawnRequest* getThreadSpawnReq();
   void dequeueThreadSpawnReq (ThreadSpawnRequest *req);

   void terminateThreadSpawners ();

   // events
   void onThreadStart(ThreadSpawnRequest *req);
   void onThreadExit();

   // misc
   void stallThread(tile_id_t tile_id);
   void stallHelperThread(tile_id_t tile_id);
   void stallThread(core_id_t core_id);

   void resumeThread(tile_id_t tile_id);
   void resumeHelperThread(tile_id_t tile_id);
   void resumeThread(core_id_t core_id);

   bool isThreadRunning(tile_id_t core_id);
   bool isHelperThreadRunning(tile_id_t tile_id);
   bool isThreadRunning(core_id_t core_id);

   bool isThreadInitializing(tile_id_t tile_id);
   bool isHelperThreadInitializing(tile_id_t tile_id);
   bool isThreadInitializing(core_id_t core_id);
   
   bool areAllCoresRunning();

private:

   friend class LCP;
   friend class MCP;

   void masterSpawnThread(ThreadSpawnRequest*);
   void slaveSpawnThread(ThreadSpawnRequest*);
   void masterSpawnThreadReply(ThreadSpawnRequest*);

   void masterSpawnHelperThread(ThreadSpawnRequest*);

   void masterOnThreadExit(tile_id_t tile_id, UInt32 core_type, UInt64 time);

   void slaveTerminateThreadSpawnerAck (tile_id_t);
   void slaveTerminateThreadSpawner ();
   void updateTerminateThreadSpawner ();

   void masterJoinThread(ThreadJoinRequest *req, UInt64 time);
   void wakeUpWaiter(core_id_t core_id, UInt64 time);
   void wakeUpMainWaiter(core_id_t core_id, UInt64 time);
   void wakeUpHelperWaiter(core_id_t core_id, UInt64 time);

   void insertThreadSpawnRequest (ThreadSpawnRequest *req);

   struct ThreadState
   {
      Core::State status;
      //SInt32 waiter;
      core_id_t waiter;

      ThreadState()
         : status(Core::IDLE)
         , waiter(INVALID_CORE_ID)
      {} 
   };

   bool m_master;
   bool m_enable_pep_cores;
   std::vector<ThreadState> m_thread_state;
   std::vector<ThreadState> m_helper_thread_state;
   std::queue<ThreadSpawnRequest*> m_thread_spawn_list;
   Semaphore m_thread_spawn_sem;
   Lock m_thread_spawn_lock;

   TileManager *m_tile_manager;

   Lock m_thread_spawners_terminated_lock;
   UInt32 m_thread_spawners_terminated;
};

#endif // THREAD_MANAGER_H
