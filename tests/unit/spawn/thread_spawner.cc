
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "thread_spawner.h"
#include "pthread.h"
#include "simulator.h"
#include "core.h"
#include "thread_manager.h"
#include "core_manager.h"
#include "capi.h"

CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   LOG_PRINT("SimSendW - sender: %d, recv: %d", sender, receiver);

   UInt32 sending_core = Config::getSingleton()->getCoreFromCommId(sender);
   UInt32 receiving_core = Config::getSingleton()->getCoreFromCommId(receiver);

   return core ? core->coreSendW(sending_core, receiving_core, buffer, size) : -1;
}

CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   LOG_PRINT("SimRecvW - sender: %d, recv: %d", sender, receiver);

   UInt32 sending_core = Config::getSingleton()->getCoreFromCommId(sender);
   UInt32 receiving_core = Config::getSingleton()->getCoreFromCommId(receiver);

   return core ? core->coreRecvW(sending_core, receiving_core, buffer, size) : -1;
}

int CarbonSpawnThread(thread_func_t func, void *arg)
{
   return Sim()->getThreadManager()->spawnThread(func, arg);
}

void CarbonJoinThread(int tid)
{
   Sim()->getThreadManager()->joinThread(tid);
}

// Support functions provided by the simulator
void CarbonGetThreadToSpawn(ThreadSpawnRequest **req)
{
   Sim()->getThreadManager()->getThreadToSpawn(req);
}

void CarbonThreadStart(ThreadSpawnRequest *req)
{
   Sim()->getThreadManager()->onThreadStart(req);
}

void CarbonThreadExit()
{
   Sim()->getThreadManager()->onThreadExit();
}

void *CarbonSpawnManagedThread(void *p)
{
   ThreadSpawnRequest *thread_info = (ThreadSpawnRequest *)p;

   CarbonThreadStart(thread_info);

   thread_info->func(thread_info->arg);

   CarbonThreadExit();
   return NULL;
}


// This function will spawn threads provided by the sim
void *CarbonThreadSpawner(void *p)
{
   while(1)
   {
      ThreadSpawnRequest *req;

      // Wait for a spawn
      CarbonGetThreadToSpawn(&req);

      if(req->func)
      {
         pthread_t thread;
         pthread_attr_t attr;
         pthread_attr_init(&attr);
         pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

         pthread_create(&thread, &attr, CarbonSpawnManagedThread, req);
      }
      else
      {
         break;
      }
   }
   return NULL;
}

// This function spawns the thread spawner
int CarbonSpawnThreadSpawner()
{
   //FIXME: a hack to get the output working properly
   setvbuf( stdout, NULL, _IONBF, 0 );

   pthread_t thread;
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   pthread_create(&thread, &attr, CarbonThreadSpawner, NULL);
   return 0;
}
