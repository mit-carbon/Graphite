
#include "transport.h"
#if defined(USE_MPI_TRANSPORT)
#include "mpitransport.cc"
#else
#include "smtransport.cc"
#endif

