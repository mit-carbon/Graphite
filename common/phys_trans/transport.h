#define USE_MPI_TRANSPORT

#ifdef USE_MPI_TRANSPORT
#include "mpitransport.h"
#else
#include "smtransport.h"
#endif
