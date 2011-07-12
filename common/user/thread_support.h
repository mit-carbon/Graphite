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
   core_id_t requester;
   thread_id_t requester_tidx;
   core_id_t destination;
   thread_id_t destination_tidx;
   thread_id_t new_tid;
   UInt64 time;
} ThreadSpawnRequest;

typedef struct 
{
   SInt32 msg_type;
   core_id_t sender;
   thread_id_t sender_tidx;
   //core_id_t receiver;
   //thread_id_t receiver_tidx;
   thread_id_t receiver_tid;
} ThreadJoinRequest;

typedef struct 
{
   SInt32 msg_type;
   core_id_t requester;
   thread_id_t requester_tidx;
} ThreadYieldRequest;

#ifdef __cplusplus
extern "C" {
#endif

carbon_thread_t CarbonSpawnThread(thread_func_t func, void *arg);
carbon_thread_t CarbonSpawnThreadOnTile(tile_id_t tile_id, thread_func_t func, void *arg);
void CarbonJoinThread(carbon_thread_t tid);

#ifdef __cplusplus
}
#endif

#endif
