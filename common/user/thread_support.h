#ifndef THREAD_SUPPORT_H
#define THREAD_SUPPORT_H

#include "fixed_types.h"

typedef int carbon_thread_t;

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

typedef struct
{
   IntPtr base;
   UInt32 size;
} StackAttributes;

#ifdef __cplusplus
extern "C" {
#endif

SInt32 CarbonSpawnThread(thread_func_t func, void *arg);
void CarbonJoinThread(SInt32 tid);

#ifdef __cplusplus
}
#endif

#endif
