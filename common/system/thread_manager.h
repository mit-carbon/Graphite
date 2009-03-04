#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <vector>
#include <queue>
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
   void joinThread(SInt32 core_id);

   void getThreadToSpawn(ThreadSpawnRequest **req);

   // events
   void onThreadStart(ThreadSpawnRequest *req);
   void onThreadExit();

private:

   friend class LCP;
   void masterSpawnThread(ThreadSpawnRequest*);
   void slaveSpawnThread(ThreadSpawnRequest*);
   void masterSpawnThreadReply(ThreadSpawnRequest*);

   void masterOnThreadExit(SInt32 core_id, UInt64 time);

   void masterJoinThread(ThreadJoinRequest *req);
   void wakeUpWaiter(SInt32 core_id);

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
};

#endif // THREAD_MANAGER_H
