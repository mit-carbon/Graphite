// Harshad Kasture
//
// This file declares the user level message passing functions used in multithreaded applications 
// for the multicore simulator.

#ifndef CAPI_H
#define CAPI_H

typedef int CAPI_return_t;

typedef int CAPI_endpoint_t;


extern "C" {

   CAPI_return_t CAPI_Initialize(int *rank);

   CAPI_return_t CAPI_rank(int * rank);

   CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, 
                                     char * buffer, int size);

   CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, 
                                        char * buffer, int size);
}
	
	//FIXME this is a temp hack function
	CAPI_return_t CAPI_Finish(int my_rank);
#endif
