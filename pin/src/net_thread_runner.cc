
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
    SimSharedMemThreadFunc();
    int core_id = g_chip->registerSharedMemThread();
    Network *net = g_chip->getCore(core_id)->getNetwork();

    while(net_threads_continue)
    {
        net->netPullFromTransport();
    }
}

