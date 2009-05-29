#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "config.h"
#include "log.h"
#include "config.hpp"

class MCP;
class LCP;
class Transport;
class CoreManager;
class Thread;
class ThreadManager;
class SimThreadManager;
class PerformanceModeler;

class Simulator
{
public:
   Simulator();
   ~Simulator();

   void start();

   static Simulator* getSingleton();
   static void setConfig(config::Config * cfg);
   static void allocate();
   static void release();

   MCP *getMCP() { return m_mcp; }
   LCP *getLCP() { return m_lcp; }
   CoreManager *getCoreManager() { return m_core_manager; }
   SimThreadManager *getSimThreadManager() { return m_sim_thread_manager; }
   ThreadManager *getThreadManager() { return m_thread_manager; }
   PerformanceModeler *getPerformanceModeler() { return m_performance_modeler; }
   Config *getConfig() { return &m_config; }
   config::Config *getCfg() { return m_config_file; }

   bool finished();

private:

   void startMCP();
   void endMCP();

   // handle synchronization of shutdown for distributed simulator objects
   void broadcastFinish();
   void handleFinish(); // slave processes
   void deallocateProcess(); // master process
   friend class LCP;

   MCP *m_mcp;
   Thread *m_mcp_thread;

   LCP *m_lcp;
   Thread *m_lcp_thread;

   Config m_config;
   Log m_log;
   Transport *m_transport;
   CoreManager *m_core_manager;
   ThreadManager *m_thread_manager;
   SimThreadManager *m_sim_thread_manager;
   PerformanceModeler *m_performance_modeler;

   static Simulator *m_singleton;

   bool m_finished;
   UInt32 m_num_procs_finished;
   
   static config::Config *m_config_file;
};

__attribute__((unused)) static Simulator *Sim()
{
   return Simulator::getSingleton();
}

#endif // SIMULATOR_H
