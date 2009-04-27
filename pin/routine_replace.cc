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

int CarbonPthreadCreateWrapperReplacement(CONTEXT *ctx, AFUNPTR orig_fp, void *pthread_t_p, void *pthread_attr_t_p, void *routine_p, void* arg_p)
{
   fprintf(stderr, "Create pthread called from pin.\n");
   // Get the function for the thread spawner
   PIN_LockClient();
   AFUNPTR pthread_create_function;
   IMG img = IMG_FindByAddress((ADDRINT)orig_fp);
   RTN rtn = RTN_FindByName(img, "pthread_create");
   pthread_create_function = RTN_Funptr(rtn);
   PIN_UnlockClient();

   fprintf(stderr, "pthread_create_function: %x\n", (int)pthread_create_function);

   int res;
   PIN_CallApplicationFunction(ctx,
         PIN_ThreadId(),
         CALLINGSTD_DEFAULT,
         pthread_create_function,
         PIN_PARG(int), &res,
         PIN_PARG(void*), pthread_t_p,
         PIN_PARG(void*), pthread_attr_t_p,
         PIN_PARG(void*), routine_p,
         PIN_PARG(void*), arg_p,
         PIN_PARG_END());

   return res;
}

void CarbonPthreadExitExceptNot(void *)
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
   else if (name == "CarbonMutexInit") msg_ptr = AFUNPTR(CarbonMutexInit);
   else if (name == "CarbonMutexLock") msg_ptr = AFUNPTR(CarbonMutexLock);
   else if (name == "CarbonMutexUnlock") msg_ptr = AFUNPTR(CarbonMutexUnlock);
   else if (name == "CarbonCondInit") msg_ptr = AFUNPTR(CarbonCondInit);
   else if (name == "CarbonCondWait") msg_ptr = AFUNPTR(CarbonCondWait);
   else if (name == "CarbonCondSignal") msg_ptr = AFUNPTR(CarbonCondSignal);
   else if (name == "CarbonCondBroadcast") msg_ptr = AFUNPTR(CarbonCondBroadcast);
   else if (name == "CarbonBarrierInit") msg_ptr = AFUNPTR(CarbonBarrierInit);
   else if (name == "CarbonBarrierWait") msg_ptr = AFUNPTR(CarbonBarrierWait);

   // pthread wrappers
//   else if (name == "CarbonPthreadCreateWrapper") msg_ptr = AFUNPTR(CarbonPthreadCreateWrapperReplacement);
//   else if (name.find("pthread_create") != std::string::npos) msg_ptr = AFUNPTR(CarbonPthreadCreate);
//   else if (name.find("pthread_join") != std::string::npos) msg_ptr = AFUNPTR(CarbonPthreadJoin);
   else if (name.find("pthread_exit") != std::string::npos) msg_ptr = AFUNPTR(CarbonPthreadExitExceptNot);

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
   else if(msg_ptr == AFUNPTR(CarbonPthreadCreateWrapperReplacement))
   {
      proto = PROTO_Allocate(PIN_PARG(int),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(void*),
                             PIN_PARG(void*),
                             PIN_PARG(void*),
                             PIN_PARG(void*),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_PROTOTYPE, proto,
                           IARG_CONTEXT,
                           IARG_ORIG_FUNCPTR,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
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
