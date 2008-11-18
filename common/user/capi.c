#include "capi.h"


// Stub implementations of user level message passing functions declared in capi.h
// These functions are replaced by the pintool with functions that implement the 
// corresponding message passing primitives.

CAPI_return_t CAPI_Initialize(int rank)
{ 
   rank = 0;
   return 0; 
}

CAPI_return_t CAPI_Initialize_FreeRank(int *rank)
{ 
   *rank = 0;
   return 0; 
}

CAPI_return_t CAPI_rank(int *rank)
{
   *rank = 0;
   return 0;
}

CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, 
                                  char * buffer, int size)
{ 
   return 0; 
}

CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, 
                                     char * buffer, int size)
{ 
   return 0; 
}
