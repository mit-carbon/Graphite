#include "routine_replace.h"
#include "simulator.h"
#include "thread_manager.h"
#include "log.h"
#include "carbon_user.h"
#include "thread_support_private.h"

int CarbonStartSimNull()
{
   return 0;
}

void CarbonStopSimNull()
{
}

bool replaceUserAPIFunction(RTN& rtn, string& name)
{
   AFUNPTR msg_ptr = NULL;
   PROTO proto = NULL;

   // thread management
   if (name == "CarbonGetThreadToSpawn") msg_ptr = AFUNPTR(CarbonGetThreadToSpawn);
   else if (name == "CarbonGetThreadSpawnReq") msg_ptr = AFUNPTR (CarbonGetThreadSpawnReq);
   // else if (name == "CarbonThreadStart") msg_ptr = AFUNPTR(CarbonThreadStart);
   // else if (name == "CarbonThreadExit") msg_ptr = AFUNPTR(CarbonThreadExit);
   else if (name == "CarbonGetCoreId") msg_ptr = AFUNPTR(CarbonGetCoreId);

   // Carbon API
   else if (name == "CarbonStartSim") msg_ptr = AFUNPTR(CarbonStartSimNull);
   else if (name == "CarbonStopSim") msg_ptr = AFUNPTR(CarbonStopSimNull);
   else if (name == "CarbonSpawnThread") msg_ptr = AFUNPTR(CarbonSpawnThread);
   else if (name == "CarbonJoinThread") msg_ptr = AFUNPTR(CarbonJoinThread);

   // CAPI
   else if (name == "CAPI_Initialize") msg_ptr = AFUNPTR(CAPI_Initialize);
   else if (name == "CAPI_rank") msg_ptr = AFUNPTR(CAPI_rank);
   else if (name == "CAPI_message_send_w") msg_ptr = AFUNPTR(CAPI_message_send_w);
   else if (name == "CAPI_message_receive_w") msg_ptr = AFUNPTR(CAPI_message_receive_w);

   // synchronization
   else if (name == "mutexInit") msg_ptr = AFUNPTR(CarbonMutexInit);
   else if (name == "mutexLock") msg_ptr = AFUNPTR(CarbonMutexLock);
   else if (name == "mutexUnlock") msg_ptr = AFUNPTR(CarbonMutexUnlock);
   else if (name == "condInit") msg_ptr = AFUNPTR(CarbonCondInit);
   else if (name == "condWait") msg_ptr = AFUNPTR(CarbonCondWait);
   else if (name == "condSignal") msg_ptr = AFUNPTR(CarbonCondSignal);
   else if (name == "condBroadcast") msg_ptr = AFUNPTR(CarbonCondBroadcast);
   else if (name == "barrierInit") msg_ptr = AFUNPTR(CarbonBarrierInit);
   else if (name == "barrierWait") msg_ptr = AFUNPTR(CarbonBarrierWait);

   if (msg_ptr != NULL)
   {
      RTN_Replace(rtn, msg_ptr);
      return true;
   }
   else
   {
      return false;
   }
}
