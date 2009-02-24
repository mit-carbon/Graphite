#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <vector>

#include "fixed_types.h"
#include "cond.h"

class CoreManager;

class ThreadManager
{
public:
   ThreadManager(CoreManager*);
   ~ThreadManager();

   // services
   SInt32 spawnThread(void (*func)(void*), void *arg);
   void joinThread(SInt32 core_id);

private:
   // events
   void onThreadStart(SInt32 core_id);
   void onThreadExit();


   struct ThreadSpawnRequest
   {
      SInt32 msg_type;
      void (*func)(void*);
      void *arg;
      SInt32 requester;
      SInt32 core_id;
   };

   friend class LCP;
   void masterSpawnThread(ThreadSpawnRequest*);
   void slaveSpawnThread(ThreadSpawnRequest*);
   void masterSpawnThreadReply(ThreadSpawnRequest*);
   static void spawnedThreadFunc(void *vpreq);

   void masterOnThreadExit(SInt32 core_id, UInt64 time);

   void masterJoinThread(SInt32 core_id, SInt32 caller);

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

   CoreManager *m_core_manager;
};

#endif // THREAD_MANAGER_H
