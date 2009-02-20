#include "simulator.h"
#include "log.h"
#include "lcp.h"
#include "mcp.h"
#include "core.h"
#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE SIMULATOR

Simulator *Simulator::m_singleton;

void Simulator::allocate()
{
   assert(m_singleton == NULL);
   m_singleton = new Simulator();
}

void Simulator::release()
{
   delete m_singleton;
   m_singleton = NULL;
}

Simulator* Simulator::getSingleton()
{
   return m_singleton;
}

Simulator::Simulator()
   : m_config()
   , m_log(m_config.getTotalCores())
   , m_transport(NULL)
   , m_core_manager(NULL)
{
}

void Simulator::start()
{
   LOG_PRINT("In Simulator ctor.");

   m_transport = Transport::create();

   m_core_manager = new CoreManager();

   m_config.logCoreMap();

   LOG_ASSERT_ERROR_EXPLICIT(!m_config.isSimulatingSharedMemory() || m_config.getEnableDCacheModeling(), -1, PINSIM,
                             "*ERROR* Must set dcache modeling on (-mdc) to use shared memory model.");

   startMCP();

   m_sim_thread_manager.spawnSimThreads();

   m_lcp = new LCP();
   m_lcp_thread = Thread::create(m_lcp);
   m_lcp_thread->run();
}

Simulator::~Simulator()
{
   LOG_PRINT("Simulator dtor starting...");

   m_transport->barrier();

   endMCP();

   m_sim_thread_manager.quitSimThreads();

   m_core_manager->outputSummary();

   delete m_lcp_thread;
   delete m_mcp_thread;
   delete m_lcp;
   delete m_mcp;
   delete m_transport;
   delete m_core_manager;

   LOG_PRINT("Simulator dtor finished.");
}

void Simulator::startMCP()
{
   if (m_config.getCurrentProcessNum() != m_config.getProcessNumForCore(Config::getSingleton()->getMCPCoreNum()))
      return;
      
   LOG_PRINT("Creating new MCP object in process %i", m_config.getCurrentProcessNum());

   Core * mcp_core = m_core_manager->getCoreFromID(m_config.getMCPCoreNum());
   LOG_ASSERT_ERROR(mcp_core, "Could not find the MCP's core!");

   Network & mcp_network = *(mcp_core->getNetwork());
   m_mcp = new MCP(mcp_network);

   m_mcp_thread = Thread::create(m_mcp);
   m_mcp_thread->run();
}

void Simulator::endMCP()
{
   if (m_config.getCurrentProcessNum() == m_config.getProcessNumForCore(m_config.getMCPCoreNum()))
      m_mcp->finish();

   m_transport->barrier();
}
