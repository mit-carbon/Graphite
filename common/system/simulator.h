#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "mcp.h"
#include "lcp.h"
#include "core_manager.h"
#include "config.h"
#include "log.h"
#include "transport.h"
#include "sim_thread_manager.h"

class Simulator
{
public:
   Simulator();
   ~Simulator();

   static Simulator* getSingleton();
   static void allocate();
   static void release();

   MCP *getMCP() { return m_mcp; }
   LCP *getLCP() { return m_lcp; }
   CoreManager *getCoreManager() { return &m_core_manager; }
   SimThreadManager *getSimThreadManager() { return &m_sim_thread_manager; }
   Config *getConfig() { return &m_config; }

private:

   void startMCP();
   void endMCP();

   MCP *m_mcp;
   Thread *m_mcp_thread;

   LCP *m_lcp;
   Thread *m_lcp_thread;

   SimThreadManager m_sim_thread_manager;

   CoreManager m_core_manager;
   Config m_config;
   Log m_log;
   Transport *m_transport;

   static Simulator *m_singleton;
};

Simulator *Sim()
{
   return Simulator::getSingleton();
}

#endif // SIMULATOR_H