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
   void getThreadSpawnReq (ThreadSpawnRequest *req);

   // TODO: Move this to somewhere better
   SInt32 getCoreIndexFromStackPtr(IntPtr stack_ptr)
   {
      IntPtr stack_base = Config::getSingleton()->getStackBase();
      UInt32 stack_size_per_core = Config::getSingleton()->getStackSizePerCore();
      UInt32 num_local_cores = Config::getSingleton()->getNumLocalCores();
      IntPtr stack_limit = stack_base + stack_size_per_core * num_local_cores;

      if ( (stack_ptr < stack_base) || (stack_ptr > stack_limit) )
      {
         return -1;
      }     
      
      return (SInt32) ((stack_ptr - stack_base) / stack_size_per_core);
   }

   // // events
   // void onThreadStart(ThreadSpawnRequest *req);
   // void onThreadExit();

   // events
   void threadStart ();
   void threadFini ();

private:

   friend class LCP;
   void masterSpawnThread(ThreadSpawnRequest*);
   void slaveSpawnThread(ThreadSpawnRequest*);
   void masterSpawnThreadReply(ThreadSpawnRequest*);

   void masterOnThreadExit(core_id_t core_id, UInt64 time);

   void masterJoinThread(ThreadJoinRequest *req);
   void wakeUpWaiter(core_id_t core_id);

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

   std::map<core_id_t, ThreadSpawnRequest*> m_thread_req_map;
   Lock m_thread_req_map_lock;

   CoreManager *m_core_manager;
};

#endif // THREAD_MANAGER_H
