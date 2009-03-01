// Harshad Kasture
//
// This file declares the user level message passing functions used in multithreaded applications
// for the multicore simulator.

#ifndef CAPI_H
#define CAPI_H

typedef int CAPI_return_t;

typedef int CAPI_endpoint_t;

typedef int carbon_thread_t;

#ifdef __cplusplus
extern "C"
{
#endif

   //FIXME: put me some place better
   int CarbonSpawnThread(void* (*func)(void*), void *arg);
   void CarbonJoinThread(int tid);
   int CarbonGetProcessCount();
   int CarbonGetCurrentProcessId();

   CAPI_return_t CAPI_Initialize(int rank);

   CAPI_return_t CAPI_rank(int *rank);

   CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint,
                                     char * buffer, int size);

   CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint,
                                        char * buffer, int size);

#ifdef __cplusplus
}
#endif


#endif
