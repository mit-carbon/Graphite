#include "sim_thread_runner.h"
#include "sim_thread.h"

// This is the function defined in chip.cc
// that represents the shared mem thread function
extern void* SimSharedMemThreadFunc(void *);

SimThreadRunner::SimThreadRunner()
{
}

void SimThreadRunner::RunThread(OS_SERVICES::ITHREAD *me)
{
   SimThreadFunc(NULL);
}

