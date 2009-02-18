// Harshad Kasture
//
// This file declares the user level message passing functions used in multithreaded applications
// for the multicore simulator.

#ifndef CAPI_H
#define CAPI_H




typedef int CAPI_return_t;

typedef int CAPI_endpoint_t;
// FIXME: Hack for debugging purposes
enum addr_t
{
   DRAM_0,
   DRAM_1,
   DRAM_00,
   DRAM_01,
   DRAM_10,
   DRAM_11,
   NUM_ADDR_TYPES
};


#ifdef __cplusplus
// externed so the names don't get name-mangled
extern "C"
{
#endif

   //FIXME: put me some place better
   void carbonInitializeThread();

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
