#include "sim_thread_manager.h"

#include "log.h"

#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE SIM_THREAD_MANAGER

SimThreadManager::SimThreadManager()
   : m_active_threads_lock(Lock::create())
   , m_active_threads(0)
{
}

SimThreadManager::~SimThreadManager()
{
   assert(m_active_threads == 0);
   delete m_active_threads_lock;
}

void SimThreadManager::spawnSimThreads()
{
   UInt32 num_sim_threads = g_config->getNumLocalCores();

   LOG_PRINT("Starting %d threads on proc: %d\n.", num_sim_threads, g_config->getCurrentProcessNum());

   m_sim_threads = new SimThread [num_sim_threads];

   for (UInt32 i = 0; i < num_sim_threads; i++)
   {
      LOG_PRINT("Starting thread %i", i);
      m_sim_threads[i]->spawn();
   }

   while (m_active_threads < num_sim_threads)
      sched_yield();

   LOG_PRINT("Threads started.");
}

void SimThreadManager::quitSimThreads()
{
   // FIXME: This should use the LCP to communicate to all the local
   // cores so that each process handles termination itself.

   if (Sim()->getConfig()->getCurrentProcessNum() 
       == 
       Sim()->getConfig()->getProcessNumForCore(Sim()->getConfig()->getMCPCoreNum()))
   {
      NetPacket pkt;
      pkt.type = SIM_THREAD_TERMINATE_THREADS;
      pkt.length = 0;
      pkt.data = 0;

      LOG_PRINT("Sending quit messages.");

      Sim()->getMCP()->broadcastPacket(pkt);
   }

   LOG_PRINT("Waiting for local sim threads to exit.");

   while (m_active_threads > 0)
      sched_yield();

   Transport::getSingleton()->barrier();
}

void SimThreadManager::simThreadStartCallback()
{
   g_sim_threads_lock->acquire();
   ++g_sim_num_active_threads;
   g_sim_threads_lock->release();
}

void SimThreadManager::simThreadExitCallback()
{
   g_sim_threads_lock->acquire();
   --g_sim_num_active_threads;
   g_sim_threads_lock->release();
}
