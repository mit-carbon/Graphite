// Harshad Kasture
//
// This file declares the user level message passing functions used in multithreaded applications
// for the multicore simulator.

#ifndef CAPI_H
#define CAPI_H

#ifdef __cplusplus
extern "C"
{
#endif

   typedef int CAPI_return_t;

   typedef int CAPI_endpoint_t;

   CAPI_return_t CAPI_Initialize(int rank);
   CAPI_return_t CAPI_rank(int *rank);
   CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint,
                                     char * buffer, int size);
   CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint,
                                        char * buffer, int size);

   enum {
      CAPI_StatusOk,
      CAPI_SenderNotInitialized,
      CAPI_ReceiverNotInitialized
   };

#ifdef __cplusplus
}
#endif

#endif
