
#define USE_MPI_TRANSPORT

#if defined(USE_MPI_TRANSPORT)
#include "mpitransport.h"
#else
#include "smtransport.h"
#endif
