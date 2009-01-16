
#include "net_thread_runner.h"
#include "chip.h"

// This is the function defined in chip.cc
// that represents the shared mem thread function
extern void* SimSharedMemThreadFunc(void *);

NetThreadRunner::NetThreadRunner()
{
}

void NetThreadRunner::RunThread(OS_SERVICES::ITHREAD *me)
{
    SimSharedMemThreadFunc(NULL);
}

