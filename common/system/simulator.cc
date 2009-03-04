#include "simulator.h"
#include "log.h"
#include "lcp.h"
#include "mcp.h"
#include "core.h"
#include "core_manager.h"
#include "thread_manager.h"
#include "sim_thread_manager.h"

#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE SIMULATOR

Simulator *Simulator::m_singleton;
config::Config *Simulator::m_config_file;

void Simulator::allocate()
{
   assert(m_singleton == NULL);
   m_singleton = new Simulator();
}

void Simulator::setConfig(config::Config *cfg)
{
   m_config_file = cfg;
}

void Simulator::release()
{
   delete m_singleton;
   m_singleton = NULL;

   //FIXME FIXME FIXME
   //For some reason there is an issue with
   //a global destructor, so once the simulator
   //has been released, we use this internal _exit
   //to avoid that destructor until the bug is
   //fixed.
   //FIXME FIXME FIXME
   _exit(0);
}

Simulator* Simulator::getSingleton()
{
   return m_singleton;
}

Simulator::Simulator()
   : m_mcp(NULL)
   , m_mcp_thread(NULL)
   , m_lcp(NULL)
   , m_lcp_thread(NULL)
   , m_config()
   , m_log(m_config.getTotalCores())
   , m_transport(NULL)
   , m_core_manager(NULL)
   , m_thread_manager(NULL)
   , m_sim_thread_manager(NULL)
   , m_finished(false)
{
}

void Simulator::start()
{
   LOG_PRINT("In Simulator ctor.");

   m_config.logCoreMap();

   m_transport = Transport::create();
   m_core_manager = new CoreManager();
   m_thread_manager = new ThreadManager(m_core_manager);
   m_sim_thread_manager = new SimThreadManager();

   startMCP();

   m_sim_thread_manager->spawnSimThreads();

   m_lcp = new LCP();
   m_lcp_thread = Thread::create(m_lcp);
   m_lcp_thread->run();
}

Simulator::~Simulator()
{
   LOG_PRINT("Simulator dtor starting...");

   broadcastFinish();

   endMCP();

   m_sim_thread_manager->quitSimThreads();

   m_transport->barrier();

   m_lcp->finish();

   m_core_manager->outputSummary();

   delete m_lcp_thread;
   delete m_mcp_thread;
   delete m_lcp;
   delete m_mcp;
   delete m_sim_thread_manager;
   delete m_thread_manager;
   delete m_core_manager;
   delete m_transport;

   LOG_PRINT("Simulator dtor finished.");
}

void Simulator::broadcastFinish()
{
   if (Config::getSingleton()->getCurrentProcessNum() != 0)
      return;

   m_num_procs_finished = 1;

   // let the rest of the simulator know its time to exit
   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();

   SInt32 msg = LCP_MESSAGE_SIMULATOR_FINISHED;
   for (UInt32 i = 1; i < Config::getSingleton()->getProcessCount(); i++)
   {
      globalNode->globalSend(i, &msg, sizeof(msg));
   }

   while (m_num_procs_finished < Config::getSingleton()->getProcessCount())
   {
      sched_yield();
   }
}

void Simulator::handleFinish()
{
   LOG_ASSERT_ERROR(Config::getSingleton()->getCurrentProcessNum() != 0,
                    "LCP_MESSAGE_SIMULATOR_FINISHED received on master process.");

   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();
   SInt32 msg = LCP_MESSAGE_SIMULATOR_FINISHED_ACK;
   globalNode->globalSend(0, &msg, sizeof(msg));

   m_finished = true;
}

void Simulator::deallocateProcess()
{
   LOG_ASSERT_ERROR(Config::getSingleton()->getCurrentProcessNum() == 0,
                    "LCP_MESSAGE_SIMULATOR_FINISHED_ACK received on slave process.");

   ++m_num_procs_finished;
}

void Simulator::startMCP()
{
   if (m_config.getCurrentProcessNum() != m_config.getProcessNumForCore(Config::getSingleton()->getMCPCoreNum()))
      return;

   LOG_PRINT("Creating new MCP object in process %i", m_config.getCurrentProcessNum());

   // FIXME: Can't the MCP look up its network itself in the
   // constructor?
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
}

bool Simulator::finished()
{
   return m_finished;
}
