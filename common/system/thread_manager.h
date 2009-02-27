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

   void getThreadToSpawn(thread_func_t *func, void **arg, int *core_id);

   // events
   void onThreadStart(SInt32 core_id);
   void onThreadExit();

private:
   struct ThreadJoinRequest
   {
       SInt32 msg_type;
       SInt32 sender;
       SInt32 core_id;

       ThreadJoinRequest()
           : msg_type(LCP_MESSAGE_THREAD_JOIN_REQUEST)
       {}
   };

   struct ThreadSpawnRequest
   {
      SInt32 msg_type;
      thread_func_t func;
      void *arg;
      SInt32 requester;
      SInt32 core_id;
   };

   friend class LCP;
   void masterSpawnThread(ThreadSpawnRequest*);
   void slaveSpawnThread(ThreadSpawnRequest*);
   void masterSpawnThreadReply(ThreadSpawnRequest*);

   void masterOnThreadExit(SInt32 core_id, UInt64 time);

   void masterJoinThread(ThreadJoinRequest *req);

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
   std::queue<ThreadSpawnRequest> m_thread_spawn_list;
   Semaphore m_thread_spawn_sem;
   Lock *m_thread_spawn_lock;

   CoreManager *m_core_manager;
};

#endif // THREAD_MANAGER_H
