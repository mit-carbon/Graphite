#include <string>
#include <map>
using namespace std;

#include "lite/routine_replace.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "log.h"
#include "dvfs.h"

// The Pintool can easily read from application memory, so
// we dont need to explicitly initialize stuff and do a special ret

namespace lite
{

map<carbon_thread_t, pthread_t> _carbon_thread_id_to_posix_thread_id_map;
Lock _map_lock;

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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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
      if (! Sim()->getCfg()->getBool("general/trigger_models_within_application", false))
      {
         RTN_InsertCall(rtn, IPOINT_BEFORE,
               AFUNPTR(Simulator::enablePerformanceModelsInCurrentProcess),
               IARG_END);
      }

      // After main()
      if (! Sim()->getCfg()->getBool("general/trigger_models_within_application", false))
      {
         RTN_InsertCall(rtn, IPOINT_AFTER,
               AFUNPTR(Simulator::disablePerformanceModelsInCurrentProcess),
               IARG_END);
      }

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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
   }

   // Get Tile ID
   else if (rtn_name == "CarbonGetTileId")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "CarbonGetTileId",
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonGetTileId),
            IARG_PROTOTYPE, proto,
            IARG_END);

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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
            IARG_ORIG_FUNCPTR,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
            IARG_END);

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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
            IARG_ORIG_FUNCPTR,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_END);

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
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
            PIN_PARG_ENUM(carbon_network_t),
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

      PROTO_Free(proto);
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
            PIN_PARG_ENUM(carbon_network_t),
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

      PROTO_Free(proto);
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

      PROTO_Free(proto);
   }

   // Dynamic voltage frequency updating
   else if (rtn_name == "CarbonGetDVFSDomain")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(int),
            CALLINGSTD_DEFAULT,
            "CarbonGetDVFSDomain",
            PIN_PARG_ENUM(module_t),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonGetDVFSDomain),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_END);

      PROTO_Free(proto);
   }

   // Dynamic voltage frequency updating
   else if (rtn_name == "CarbonGetDVFS")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(int),
            CALLINGSTD_DEFAULT,
            "CarbonGetDVFS",
            PIN_PARG(tile_id_t),
            PIN_PARG_ENUM(module_t),
            PIN_PARG(double*),
            PIN_PARG(double*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonGetDVFS),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
            IARG_END);

      PROTO_Free(proto);
   }

   else if (rtn_name == "CarbonGetFrequency")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(int),
            CALLINGSTD_DEFAULT,
            "CarbonGetFrequency",
            PIN_PARG(tile_id_t),
            PIN_PARG_ENUM(module_t),
            PIN_PARG(double*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonGetFrequency),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_END);

      PROTO_Free(proto);
   }

   else if (rtn_name == "CarbonGetVoltage")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(int),
            CALLINGSTD_DEFAULT,
            "CarbonGetVoltage",
            PIN_PARG(tile_id_t),
            PIN_PARG_ENUM(module_t),
            PIN_PARG(double*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonGetVoltage),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_END);

      PROTO_Free(proto);
   }
   
   else if (rtn_name == "CarbonSetDVFS")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(int),
            CALLINGSTD_DEFAULT,
            "CarbonSetDVFS",
            PIN_PARG(tile_id_t),
            PIN_PARG(int),
            PIN_PARG(double*),
            PIN_PARG_ENUM(voltage_option_t),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonSetDVFS),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
            IARG_END);

      PROTO_Free(proto);
   }

   else if (rtn_name == "CarbonSetDVFSAllTiles")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(int),
            CALLINGSTD_DEFAULT,
            "CarbonSetDVFSAllTiles",
            PIN_PARG(int),
            PIN_PARG(double*),
            PIN_PARG_ENUM(voltage_option_t),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonSetDVFSAllTiles),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_END);

      PROTO_Free(proto);
   }

   else if (rtn_name == "CarbonGetTileEnergy")
   {
      PROTO proto = PROTO_Allocate(PIN_PARG(int),
            CALLINGSTD_DEFAULT,
            "CarbonGetTileEnergy",
            PIN_PARG(tile_id_t),
            PIN_PARG(double*),
            PIN_PARG_END());

      RTN_ReplaceSignature(rtn,
            AFUNPTR(CarbonGetTileEnergy),
            IARG_PROTOTYPE, proto,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_END);

      PROTO_Free(proto);
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
  
   carbon_thread_t carbon_thread_id = CarbonSpawnThread(thread_func, arg);

   __attribute__((unused)) ADDRINT reg_inst_ptr = PIN_GetContextReg(context, REG_INST_PTR);
   AFUNPTR pthread_create_func_ptr = getFunptr(context, "pthread_create");
   LOG_ASSERT_ERROR(pthread_create_func_ptr, "Could not find pthread_create at instruction(%#lx)", reg_inst_ptr);

   int ret;
   pthread_t posix_thread_id;
   PIN_CallApplicationFunction(context, PIN_ThreadId(),
         CALLINGSTD_DEFAULT,
         pthread_create_func_ptr,
         PIN_PARG(int), &ret,
         PIN_PARG(pthread_t*), &posix_thread_id,
         PIN_PARG(pthread_attr_t*), NULL,
         PIN_PARG(void* (*)(void*)), thread_func,
         PIN_PARG(void*), arg,
         PIN_PARG_END());

   LOG_ASSERT_ERROR(ret == 0, "pthread_create() returned(%i)", ret);

   _map_lock.acquire();   
   _carbon_thread_id_to_posix_thread_id_map.insert(make_pair<carbon_thread_t, pthread_t>(carbon_thread_id, posix_thread_id));
   _map_lock.release();

   return carbon_thread_id;
}

int emuPthreadCreate(CONTEXT* context, AFUNPTR pthread_create_func_ptr,
      pthread_t* posix_thread_ptr, pthread_attr_t* attr,
      thread_func_t thread_func, void* arg)
{
   carbon_thread_t carbon_thread_id = CarbonSpawnThread(thread_func, arg);
  
   int ret;
   PIN_CallApplicationFunction(context, PIN_ThreadId(),
         CALLINGSTD_DEFAULT,
         pthread_create_func_ptr,
         PIN_PARG(int), &ret,
         PIN_PARG(pthread_t*), posix_thread_ptr,
         PIN_PARG(pthread_attr_t*), attr,
         PIN_PARG(void* (*)(void*)), thread_func,
         PIN_PARG(void*), arg,
         PIN_PARG_END());

   LOG_ASSERT_ERROR(ret == 0, "pthread_create() returned(%i)", ret);

   _map_lock.acquire();
   _carbon_thread_id_to_posix_thread_id_map.insert(make_pair<carbon_thread_t, pthread_t>(carbon_thread_id, *posix_thread_ptr));
   _map_lock.release();

   return ret;
}

void emuCarbonJoinThread(CONTEXT* context,
      carbon_thread_t carbon_thread_id)
{
   _map_lock.acquire();

   map<carbon_thread_t, pthread_t>::iterator it;   
   it = _carbon_thread_id_to_posix_thread_id_map.find(carbon_thread_id);

   LOG_ASSERT_ERROR(it != _carbon_thread_id_to_posix_thread_id_map.end(),
                    "Cant find pthread ID for carbon thread ID(%i)", carbon_thread_id);
   
   pthread_t posix_thread_id = it->second;

   _carbon_thread_id_to_posix_thread_id_map.erase(it);
   
   _map_lock.release();

   // Do thread cleanup functions in Graphite
   CarbonJoinThread(carbon_thread_id);

   __attribute__((unused)) ADDRINT reg_inst_ptr = PIN_GetContextReg(context, REG_INST_PTR);
   AFUNPTR pthread_join_func_ptr = getFunptr(context, "pthread_join");
   LOG_ASSERT_ERROR(pthread_join_func_ptr != NULL, "Could not find pthread_join at instruction(%#lx)", reg_inst_ptr);

   int ret;
   PIN_CallApplicationFunction(context, PIN_ThreadId(),
         CALLINGSTD_DEFAULT,
         pthread_join_func_ptr,
         PIN_PARG(int), &ret,
         PIN_PARG(pthread_t), posix_thread_id,
         PIN_PARG(void**), NULL,
         PIN_PARG_END());

   LOG_ASSERT_ERROR(ret == 0, "pthread_join() returned(%i)", ret);
}

int emuPthreadJoin(CONTEXT* context, AFUNPTR pthread_join_func_ptr,
      pthread_t posix_thread_id, void** thread_return)
{
   _map_lock.acquire();

   carbon_thread_t carbon_thread_id = INVALID_THREAD_ID;

   map<carbon_thread_t, pthread_t>::iterator it = _carbon_thread_id_to_posix_thread_id_map.begin();
   for ( ; it != _carbon_thread_id_to_posix_thread_id_map.end(); it++)
   {
      if (pthread_equal(it->second, posix_thread_id) != 0)
      {
         carbon_thread_id = it->first;
         break;
      }
   }
   LOG_ASSERT_ERROR(carbon_thread_id != INVALID_THREAD_ID, "Could not find carbon thread id");

   _carbon_thread_id_to_posix_thread_id_map.erase(it);

   _map_lock.release();

   // Complete the thread cleanup functions in Graphite
   CarbonJoinThread(carbon_thread_id);

   int ret;
   PIN_CallApplicationFunction(context, PIN_ThreadId(),
         CALLINGSTD_DEFAULT,
         pthread_join_func_ptr,
         PIN_PARG(int), &ret,
         PIN_PARG(pthread_t), posix_thread_id,
         PIN_PARG(void**), thread_return,
         PIN_PARG_END());

   // We dont need to delete the thread descriptor

   return ret;
}

IntPtr nullFunction()
{
   LOG_PRINT("In nullFunction()");
   return IntPtr(0);
}

}
