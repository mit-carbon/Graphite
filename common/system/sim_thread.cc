#include "sim_thread.h"

#include "log.h"
#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE SIM_THREAD

SimThread::SimThread()
   : m_thread(NULL)
{
}

SimThread::~SimThread()
{
   delete m_thread;
}

void SimThread::run()
{
   int core_id = Sim()->getCoreManager()->registerSimMemThread();
   Network *net = Sim()->getCoreManager()->getCoreFromID(core_id)->getNetwork();
   bool cont = true;

   Sim()->getSimThreadManager()->simThreadStartCallback();

   // Turn off cont when we receive a quit message
   net->registerCallback(SIM_THREAD_TERMINATE_THREADS,
                         terminateFunc,
                         &cont);

   // Actual work gets done here
   while (cont)
      net->netPullFromTransport();

   Sim()->getSimThreadManager()->simThreadExitCallback();

   LOG_PRINT("Sim thread %i exiting", core_id);
}

void SimThread::spawn()
{
   m_thread = Thread::create(this);
}

void SimThread::terminateFunc(void *vp, NetPacket pkt)
{
   bool *pcont = (bool*) vp;
   *pcont = false;
}
