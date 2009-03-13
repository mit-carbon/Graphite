#include "routine_replace.h"
#include "simulator.h"
#include "thread_manager.h"
#include "log.h"
#include "carbon_user.h"
#include "thread_support_private.h"

extern int CarbonMain(CONTEXT*, AFUNPTR, int, char**);

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

   // main
   if (name == "main") msg_ptr = AFUNPTR(CarbonMain);

   // thread management
   else if (name == "CarbonGetThreadToSpawn") msg_ptr = AFUNPTR(CarbonGetThreadToSpawn);
   else if (name == "CarbonThreadStart") msg_ptr = AFUNPTR(CarbonThreadStart);
   else if (name == "CarbonThreadExit") msg_ptr = AFUNPTR(CarbonThreadExit);
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

   // pthread wrappers
   else if (name == "pthread_create") msg_ptr = AFUNPTR(CarbonPthreadCreate);
   else if (name == "pthread_join") msg_ptr = AFUNPTR(CarbonPthreadJoin);

   // actual replacement
   if (msg_ptr == AFUNPTR(CarbonMain))
   {
      proto = PROTO_Allocate(PIN_PARG(int),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(int),
                             PIN_PARG(char**),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_PROTOTYPE, proto,
                           IARG_CONTEXT,
                           IARG_ORIG_FUNCPTR,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                           IARG_END);
      PROTO_Free(proto);
      return true;
   }
   else if (msg_ptr != NULL)
   {
      RTN_Replace(rtn, msg_ptr);
      return true;
   }
   else
   {
      return false;
   }
}
