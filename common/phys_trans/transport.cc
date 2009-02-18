#include "transport.h"

#ifdef USE_MPI_TRANSPORT
#include "mpitransport.cc"
#else
#include "smtransport.cc"
#endif
