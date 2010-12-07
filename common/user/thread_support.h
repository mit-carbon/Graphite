#ifndef THREAD_SUPPORT_H
#define THREAD_SUPPORT_H

#include "fixed_types.h"

typedef SInt32 carbon_thread_t;

typedef void *(*thread_func_t)(void *);

typedef struct
{
   SInt32 msg_type;
   thread_func_t func;
   void *arg;
   //SInt32 requester;
   core_id_t requester;
   //tile_id_t tile_id;
   core_id_t destination;
   UInt64 time;
} ThreadSpawnRequest;

typedef struct 
{
   SInt32 msg_type;
   //SInt32 sender;
   //tile_id_t tile_id;
   core_id_t sender;
   core_id_t receiver;
} ThreadJoinRequest;

#ifdef __cplusplus
extern "C" {
#endif

carbon_thread_t CarbonSpawnThread(thread_func_t func, void *arg);
carbon_thread_t CarbonSpawnHelperThread(thread_func_t func, void *arg);
void CarbonJoinThread(carbon_thread_t tid);
void CarbonJoinHelperThread(carbon_thread_t tid);

#ifdef __cplusplus
}
#endif

#endif
