#include <string>
#include <map>
using namespace std;

#include "lite/routine_replace.h"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "log.h"

// The Pintool can easily read from application memory, so
// we dont need to explicitly initialize stuff and do a special ret

namespace lite
{

multimap<core_id_t, pthread_t*> tid_to_thread_ptr_map;

void routineCallback(RTN rtn, void* v)
{
   string rtn_name = RTN_Name(rtn);

   // Enable Models
   if (rtn_name == "CarbonEnableModels")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonEnableModels",
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonEnableModels),
            IARG_PROTOTYPE, proto,
            IARG_END);
   }
 
   // Disable Models
   if (rtn_name == "CarbonDisableModels")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonDisableModels",
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonDisableModels),
            IARG_PROTOTYPE, proto,
            IARG_END);
   }

   // Reset Models
   if (rtn_name == "CarbonResetModels")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonResetModels",
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonResetModels),
            IARG_PROTOTYPE, proto,
            IARG_END);
   }

   // _start
   if (rtn_name == "_start")
   {
      RTN_Open(rtn);

      RTN_InsertCall(rtn, IPOINT_BEFORE,
            AFUNPTR(Simulator::disablePerformanceModelsInCurrentProcess),
            IARG_END);

      RTN_Close(rtn);
   }

   // main
   if (rtn_name == "main")
   {
      RTN_Open(rtn);

      // Before main()
      if (Sim()->getCfg()->getBool("general/enable_models_at_startup",true))
      {
         RTN_InsertCall(rtn, IPOINT_BEFORE,
               AFUNPTR(Simulator::enablePerformanceModelsInCurrentProcess),
               IARG_END);
      }

      RTN_InsertCall(rtn, IPOINT_BEFORE,
            AFUNPTR(CarbonInitModels),
            IARG_END);

      // After main()
      RTN_InsertCall(rtn, IPOINT_AFTER,
            AFUNPTR(Simulator::disablePerformanceModelsInCurrentProcess),
            IARG_END);

      RTN_Close(rtn);
   }

   // CarbonStartSim() and CarbonStopSim()
   if (rtn_name == "CarbonStartSim")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(SInt32),
            CALLINGSTD_DEFAULT,
            "CarbonStartSim",
            PIN_PARG(int),
            PIN_PARG(char**),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(lite::nullFunction),
            IARG_PROTOTYPE, proto,
            IARG_END);
   }
   else if (rtn_name == "CarbonStopSim")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonStopSim",
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(lite::nullFunction),
            IARG_PROTOTYPE, proto,
            IARG_END);
   }

   // Thread Creation
   else if (rtn_name == "CarbonSpawnThread")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(carbon_thread_t),
            CALLINGSTD_DEFAULT,
            "CarbonSpawnThread",
            PIN_PARG(thread_func_t),
            PIN_PARG(void*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(lite::emuCarbonSpawnThread),
            IARG_PROTOTYPE, proto,
            IARG_CONTEXT,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_END);
   }
   else if (rtn_name.find("pthread_create") != string::npos)
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(int),
            CALLINGSTD_DEFAULT,
            "pthread_create",
            PIN_PARG(pthread_t*),
            PIN_PARG(pthread_attr_t*),
            PIN_PARG(void* (*)(void*)),
            PIN_PARG(void*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(lite::emuPthreadCreate),
            IARG_PROTOTYPE, proto,
            IARG_CONTEXT,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
            IARG_END);
   }
   // Thread Joining
   else if (rtn_name == "CarbonJoinThread")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonJoinThread",
            PIN_PARG(carbon_thread_t),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(lite::emuCarbonJoinThread),
            IARG_PROTOTYPE, proto,
            IARG_CONTEXT,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   else if (rtn_name.find("pthread_join") != string::npos)
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(int),
            CALLINGSTD_DEFAULT,
            "pthread_join",
            PIN_PARG(pthread_t),
            PIN_PARG(void**),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(lite::emuPthreadJoin),
            IARG_PROTOTYPE, proto,
            IARG_CONTEXT,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_END);
   }
   // Synchronization
   else if (rtn_name == "CarbonMutexInit")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonMutexInit",
            PIN_PARG(carbon_mutex_t*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonMutexInit),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   else if (rtn_name == "CarbonMutexLock")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonMutexLock",
            PIN_PARG(carbon_mutex_t*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonMutexLock),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   else if (rtn_name == "CarbonMutexUnlock")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonMutexUnlock",
            PIN_PARG(carbon_mutex_t*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonMutexUnlock),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   else if (rtn_name == "CarbonCondInit")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonCondInit",
            PIN_PARG(carbon_cond_t*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonCondInit),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   else if (rtn_name == "CarbonCondWait")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonCondWait",
            PIN_PARG(carbon_cond_t*),
            PIN_PARG(carbon_mutex_t*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonCondWait),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_END);
   }
   else if (rtn_name == "CarbonCondSignal")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonCondSignal",
            PIN_PARG(carbon_cond_t*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonCondSignal),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   else if (rtn_name == "CarbonCondBroadcast")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonCondBroadcast",
            PIN_PARG(carbon_cond_t*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonCondBroadcast),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   else if (rtn_name == "CarbonBarrierInit")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonBarrierInit",
            PIN_PARG(carbon_barrier_t*),
            PIN_PARG(unsigned int),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonBarrierInit),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_END);
   }
   else if (rtn_name == "CarbonBarrierWait")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonBarrierWait",
            PIN_PARG(carbon_barrier_t*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonBarrierWait),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   
   // CAPI Functions
   else if (rtn_name == "CAPI_Initialize")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
            CALLINGSTD_DEFAULT,
            "CAPI_Initialize",
            PIN_PARG(int),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CAPI_Initialize),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   else if (rtn_name == "CAPI_rank")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
            CALLINGSTD_DEFAULT,
            "CAPI_rank",
            PIN_PARG(int*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CAPI_rank),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   else if (rtn_name == "CAPI_message_send_w")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
            CALLINGSTD_DEFAULT,
            "CAPI_message_send_w",
            PIN_PARG(CAPI_endpoint_t),
            PIN_PARG(CAPI_endpoint_t),
            PIN_PARG(char*),
            PIN_PARG(int),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CAPI_message_send_w),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
            IARG_END);
   }
   else if (rtn_name == "CAPI_message_receive_w")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
            CALLINGSTD_DEFAULT,
            "CAPI_message_receive_w",
            PIN_PARG(CAPI_endpoint_t),
            PIN_PARG(CAPI_endpoint_t),
            PIN_PARG(char*),
            PIN_PARG(int),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CAPI_message_receive_w),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
            IARG_END);
   }
   else if (rtn_name == "CAPI_message_send_w_ex")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
            CALLINGSTD_DEFAULT,
            "CAPI_message_send_w_ex",
            PIN_PARG(CAPI_endpoint_t),
            PIN_PARG(CAPI_endpoint_t),
            PIN_PARG(char*),
            PIN_PARG(int),
            PIN_PARG(UInt32),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CAPI_message_send_w_ex),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
            IARG_END);
   }
   else if (rtn_name == "CAPI_message_receive_w_ex")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
            CALLINGSTD_DEFAULT,
            "CAPI_message_receive_w_ex",
            PIN_PARG(CAPI_endpoint_t),
            PIN_PARG(CAPI_endpoint_t),
            PIN_PARG(char*),
            PIN_PARG(int),
            PIN_PARG(UInt32),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CAPI_message_receive_w_ex),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 4,
            IARG_END);
   }

   // Getting Simulated Time
   else if (rtn_name == "CarbonGetTime")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(UInt64),
            CALLINGSTD_DEFAULT,
            "CarbonGetTime",
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonGetTime),
            IARG_PROTOTYPE, proto,
            IARG_END);
   }

   // Frequency Scaling Functions
   else if (rtn_name == "CarbonGetCoreFrequency")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonGetCoreFrequency",
            PIN_PARG(float*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonGetCoreFrequency),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
   else if (rtn_name == "CarbonSetCoreFrequency")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonSetCoreFrequency",
            PIN_PARG(float*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonSetCoreFrequency),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);
   }
}

AFUNPTR getFunptr(CONTEXT* context, string func_name)
{
   IntPtr reg_inst_ptr = PIN_GetContextReg(context, REG_INST_PTR);

   PIN_LockClient();
   IMG img = IMG_FindByAddress(reg_inst_ptr);
   RTN rtn = RTN_FindByName(img, func_name.c_str());
   PIN_UnlockClient();
   
   return RTN_Funptr(rtn);
}

carbon_thread_t emuCarbonSpawnThread(CONTEXT* context,
      thread_func_t thread_func, void* arg)
{
   LOG_PRINT("Entering emuCarbonSpawnThread(%p, %p)", thread_func, arg);
  
   core_id_t tid = CarbonSpawnThread(thread_func, arg);

   AFUNPTR pthread_create_func = getFunptr(context, "pthread_create");
   LOG_ASSERT_ERROR(pthread_create_func != NULL, "Could not find pthread_create");

   int ret;
   pthread_t* thread_ptr = new pthread_t;
   PIN_CallApplicationFunction(context, PIN_ThreadId(),
         CALLINGSTD_DEFAULT,
         pthread_create_func,
         PIN_PARG(int), &ret,
         PIN_PARG(pthread_t*), thread_ptr,
         PIN_PARG(pthread_attr_t*), NULL,
         PIN_PARG(void* (*)(void*)), thread_func,
         PIN_PARG(void*), arg,
         PIN_PARG_END());

   LOG_ASSERT_ERROR(ret == 0, "pthread_create() returned(%i)", ret);
   
   // FIXME: Figure out if we need to put a lock
   tid_to_thread_ptr_map.insert(make_pair<core_id_t, pthread_t*>(tid, thread_ptr));
   
   return tid;
}

int emuPthreadCreate(CONTEXT* context,
      pthread_t* thread_ptr, pthread_attr_t* attr,
      thread_func_t thread_func, void* arg)
{
   core_id_t tid = CarbonSpawnThread(thread_func, arg);
   
   AFUNPTR pthread_create_func = getFunptr(context, "pthread_create");
   LOG_ASSERT_ERROR(pthread_create_func != NULL, "Could not find pthread_create");

   int ret;
   PIN_CallApplicationFunction(context, PIN_ThreadId(),
         CALLINGSTD_DEFAULT,
         pthread_create_func,
         PIN_PARG(int), &ret,
         PIN_PARG(pthread_t*), thread_ptr,
         PIN_PARG(pthread_attr_t*), attr,
         PIN_PARG(void* (*)(void*)), thread_func,
         PIN_PARG(void*), arg,
         PIN_PARG_END());

   LOG_ASSERT_ERROR(ret == 0, "pthread_create() returned(%i)", ret);

   tid_to_thread_ptr_map.insert(make_pair<core_id_t, pthread_t*>(tid, thread_ptr));

   return ret;
}

void emuCarbonJoinThread(CONTEXT* context,
      carbon_thread_t tid)
{
   multimap<core_id_t, pthread_t*>::iterator it;
   
   it = tid_to_thread_ptr_map.find(tid);
   LOG_ASSERT_ERROR(it != tid_to_thread_ptr_map.end(),
         "Cant find thread_ptr for tid(%i)", tid);
   
   pthread_t* thread_ptr = it->second;

   LOG_PRINT("Starting emuCarbonJoinThread: Thread_ptr(%p), tid(%i)", thread_ptr, tid);
   
   CarbonJoinThread(tid);

   tid_to_thread_ptr_map.erase(it);

   AFUNPTR pthread_join_func = getFunptr(context, "pthread_join");
   LOG_ASSERT_ERROR(pthread_join_func != NULL, "Could not find pthread_join");

   int ret;
   PIN_CallApplicationFunction(context, PIN_ThreadId(),
         CALLINGSTD_DEFAULT,
         pthread_join_func,
         PIN_PARG(int), &ret,
         PIN_PARG(pthread_t), *thread_ptr,
         PIN_PARG(void**), NULL,
         PIN_PARG_END());

   LOG_ASSERT_ERROR(ret == 0, "pthread_join() returned(%i)", ret);
   
   LOG_PRINT("Finished emuCarbonJoinThread: Thread_ptr(%p), tid(%i)", thread_ptr, tid);
   
   // Delete the thread descriptor
   delete thread_ptr;
}

int emuPthreadJoin(CONTEXT* context,
      pthread_t thread, void** thead_return)
{
   core_id_t tid = INVALID_CORE_ID;

   multimap<core_id_t, pthread_t*>::iterator it;
   for (it = tid_to_thread_ptr_map.begin(); it != tid_to_thread_ptr_map.end(); it++)
   {
      if (pthread_equal(*(it->second), thread) != 0)
      {
         tid = it->first;
         break;
      }
   }
   LOG_ASSERT_ERROR(tid != INVALID_CORE_ID, "Could not find core_id");
  
   LOG_PRINT("Joining Thread_ptr(%p), tid(%i)", &thread, tid);

   CarbonJoinThread(tid);

   tid_to_thread_ptr_map.erase(it);

   AFUNPTR pthread_join_func = getFunptr(context, "pthread_join");
   LOG_ASSERT_ERROR(pthread_join_func != NULL, "Could not find pthread_join");

   int ret;
   PIN_CallApplicationFunction(context, PIN_ThreadId(),
         CALLINGSTD_DEFAULT,
         pthread_join_func,
         PIN_PARG(int), &ret,
         PIN_PARG(pthread_t), thread,
         PIN_PARG(void**), thead_return,
         PIN_PARG_END());

   // LOG_ASSERT_ERROR(ret == 0, "pthread_join() returned(%i)", ret);

   // We dont need to delete the thread descriptor

   return ret;
}

IntPtr nullFunction()
{
   LOG_PRINT("In nullFunction()");
   return IntPtr(0);
}

}
