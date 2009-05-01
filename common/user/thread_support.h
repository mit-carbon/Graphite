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
   SInt32 requester;
   core_id_t core_id;
} ThreadSpawnRequest;

typedef struct 
{
   SInt32 msg_type;
   SInt32 sender;
   core_id_t core_id;
} ThreadJoinRequest;

#ifdef __cplusplus
extern "C" {
#endif

carbon_thread_t CarbonSpawnThread(thread_func_t func, void *arg);
void CarbonJoinThread(carbon_thread_t tid);

#ifdef __cplusplus
}
#endif

#endif
