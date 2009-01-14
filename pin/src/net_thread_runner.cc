
#include "net_thread_runner.h"
#include "chip.h"

bool NetThreadRunner::net_threads_continue = true;

NetThreadRunner::NetThreadRunner()
{
}

void NetThreadRunner::RunThread(OS_SERVICES::ITHREAD *me)
{
    int core_id = g_chip->registerSharedMemThread();
    Network *net = g_chip->getCore(core_id)->getNetwork();

    while(net_threads_continue)
    {
        net->netPullFromTransport();
    }
}

void NetThreadRunner::StopNetThreads()
{
   net_threads_continue = false;
}

