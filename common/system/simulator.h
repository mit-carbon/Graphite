#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "config.h"
#include "log.h"
#include "config.hpp"

class MCP;
class LCP;
class Transport;
class TileManager;
class Thread;
class ThreadManager;
class ThreadScheduler;
class PerformanceCounterManager;
class SimThreadManager;
class ClockSkewManagementManager;
class StatisticsManager;
class StatisticsThread;

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
   TileManager *getTileManager() { return m_tile_manager; }
   SimThreadManager *getSimThreadManager() { return m_sim_thread_manager; }
   ThreadManager *getThreadManager() { return m_thread_manager; }
   ThreadScheduler *getThreadScheduler() { return m_thread_scheduler; }
   PerformanceCounterManager *getPerformanceCounterManager() { return m_performance_counter_manager; }
   ClockSkewManagementManager *getClockSkewManagementManager() { return m_clock_skew_management_manager; }
   StatisticsManager *getStatisticsManager() { return m_statistics_manager; } 
   StatisticsThread *getStatisticsThread() { return m_statistics_thread; } 
   Config *getConfig() { return &m_config; }
   config::Config *getCfg() { return m_config_file; }

   void startTimer();
   void stopTimer();
   bool finished();

   std::string getGraphiteHome() { return m_graphite_home; }
   
   void enableModels();
   void disableModels();

   bool isEnabled() const { return m_enabled; }

   static void enablePerformanceModelsInCurrentProcess();
   static void disablePerformanceModelsInCurrentProcess();

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
   TileManager *m_tile_manager;
   ThreadManager *m_thread_manager;
   ThreadScheduler *m_thread_scheduler;
   PerformanceCounterManager *m_performance_counter_manager;
   SimThreadManager *m_sim_thread_manager;
   ClockSkewManagementManager *m_clock_skew_management_manager;
   StatisticsManager *m_statistics_manager;
   StatisticsThread *m_statistics_thread;

   static Simulator *m_singleton;

   bool m_finished;
   UInt32 m_num_procs_finished;

   UInt64 m_boot_time;
   UInt64 m_start_time;
   UInt64 m_stop_time;
   UInt64 m_shutdown_time;
   
   static config::Config *m_config_file;

   std::string m_graphite_home;

   bool m_enabled;
};

__attribute__((unused)) static Simulator *Sim()
{
   return Simulator::getSingleton();
}

#endif // SIMULATOR_H
