#ifndef SIM_THREAD_MANAGER_H
#define SIM_THREAD_MANAGER_H

#include "sim_thread.h"

class SimThreadManager
{
public:
   SimThreadManager();
   ~SimThreadManager();

   void spawnSimThreads();
   void quitSimThreads();

   void simThreadStartCallback();
   void simThreadExitCallback();
   
private:
   SimThread *m_sim_threads;

   UInt32 m_active_threads;
   Lock *m_active_threads_lock;
};

#endif // SIM_THREAD_MANAGER
