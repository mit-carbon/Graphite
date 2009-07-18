#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <vector>
#include <queue>
#include <map>
#include "semaphore.h"

#include "fixed_types.h"
#include "message_types.h"
#include "lock.h"

#include "thread_support.h"

class CoreManager;

class ThreadManager
{
public:
   ThreadManager(CoreManager*);
   ~ThreadManager();

   // services
   SInt32 spawnThread(thread_func_t func, void *arg);
   void joinThread(core_id_t core_id);

   void getThreadToSpawn(ThreadSpawnRequest *req);
   ThreadSpawnRequest* getThreadSpawnReq();
   void dequeueThreadSpawnReq (ThreadSpawnRequest *req);

   void terminateThreadSpawners ();

   // // events
   void onThreadStart(ThreadSpawnRequest *req);
   void onThreadExit();

private:

   friend class LCP;
   friend class MCP;

   void masterSpawnThread(ThreadSpawnRequest*);
   void slaveSpawnThread(ThreadSpawnRequest*);
   void masterSpawnThreadReply(ThreadSpawnRequest*);

   void masterOnThreadExit(core_id_t core_id, UInt64 time);

   void slaveTerminateThreadSpawnerAck (core_id_t);
   void slaveTerminateThreadSpawner ();
   void updateTerminateThreadSpawner ();

   void masterJoinThread(ThreadJoinRequest *req, UInt64 time);
   void wakeUpWaiter(core_id_t core_id, UInt64 time);

   void insertThreadSpawnRequest (ThreadSpawnRequest *req);

   struct ThreadState
   {
      bool running;
      SInt32 waiter;

      ThreadState()
         : running(false)
         , waiter(-1)
      {} 
   };

   bool m_master;
   std::vector<ThreadState> m_thread_state;
   std::queue<ThreadSpawnRequest*> m_thread_spawn_list;
   Semaphore m_thread_spawn_sem;
   Lock m_thread_spawn_lock;

   CoreManager *m_core_manager;

   Lock m_thread_spawners_terminated_lock;
   UInt32 m_thread_spawners_terminated;
};

#endif // THREAD_MANAGER_H
