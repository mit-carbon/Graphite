#include <sstream>

#include "simulator.h"
#include "log.h"
#include "lcp.h"
#include "mcp.h"
#include "tile.h"
#include "tile_manager.h"
#include "thread_manager.h"
#include "perf_counter_manager.h"
#include "sim_thread_manager.h"
#include "clock_skew_minimization_object.h"
#include "fxsupport.h"
#include "contrib/orion/orion.h"

Simulator *Simulator::m_singleton;
config::Config *Simulator::m_config_file;

static UInt64 getTime()
{
   timeval t;
   gettimeofday(&t, NULL);
   UInt64 time = (((UInt64)t.tv_sec) * 1000000 + t.tv_usec);
   return time;
}

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
   // Fxsupport::fini();
   delete m_singleton;
   m_singleton = NULL;
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
   , m_log(m_config)
   , m_transport(NULL)
   , m_tile_manager(NULL)
   , m_thread_manager(NULL)
   , m_perf_counter_manager(NULL)
   , m_sim_thread_manager(NULL)
   , m_clock_skew_minimization_manager(NULL)
   , m_finished(false)
   , m_boot_time(getTime())
   , m_start_time(0)
   , m_stop_time(0)
   , m_shutdown_time(0)
{
}

void Simulator::start()
{
   LOG_PRINT("In Simulator ctor.");

   m_config.logTileMap();

   // Create Orion Config Object
   string orion_cfg_file = "./contrib/orion/orion.cfg";
   OrionConfig::allocate(orion_cfg_file);
   //OrionConfig::getSingleton()->print_config(cout);
 
   m_transport = Transport::create();
   m_tile_manager = new TileManager();
   m_thread_manager = new ThreadManager(m_tile_manager);
   m_perf_counter_manager = new PerfCounterManager(m_thread_manager);
   m_sim_thread_manager = new SimThreadManager();
   m_clock_skew_minimization_manager = ClockSkewMinimizationManager::create(getCfg()->getString("clock_skew_minimization/scheme","none"));

   //// Floating Point Support
   Fxsupport::allocate();

   startMCP();

   m_sim_thread_manager->spawnSimThreads();

   m_lcp = new LCP();
   m_lcp_thread = Thread::create(m_lcp);
   m_lcp_thread->run();

   Instruction::initializeStaticInstructionModel();

   m_transport->barrier();
}

Simulator::~Simulator()
{
   m_shutdown_time = getTime();

   LOG_PRINT("Simulator dtor starting...");

   if ((m_config.getCurrentProcessNum() == 0) && \
      (m_config.getSimulationMode() == Config::FULL))
      m_thread_manager->terminateThreadSpawners();

   broadcastFinish();

   endMCP();

   if (m_clock_skew_minimization_manager)
      delete m_clock_skew_minimization_manager;

   m_sim_thread_manager->quitSimThreads();

   m_transport->barrier();

   m_lcp->finish();

   if (Config::getSingleton()->getCurrentProcessNum() == 0)
   {
      ofstream os(Config::getSingleton()->getOutputFileName().c_str());

      os << "Simulation timers: " << endl
         << "start time\t" << (m_start_time - m_boot_time) << endl
         << "stop time\t" << (m_stop_time - m_boot_time) << endl
         << "shutdown time\t" << (m_shutdown_time - m_boot_time) << endl;

      m_tile_manager->outputSummary(os);
      os.close();
   }
   else
   {
      stringstream temp;
      m_tile_manager->outputSummary(temp);
      assert(temp.str().length() == 0);
   }

   delete m_lcp_thread;
   delete m_mcp_thread;
   delete m_lcp;
   delete m_mcp;
   delete m_sim_thread_manager;
   delete m_perf_counter_manager;
   delete m_thread_manager;
   delete m_tile_manager;
   delete m_transport;

   // Delete Orion Config Object
   OrionConfig::release();
}

void Simulator::startTimer()
{
   m_start_time = getTime();
}

void Simulator::stopTimer()
{
   m_stop_time = getTime();
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
   if (m_config.getCurrentProcessNum() != m_config.getProcessNumForTile(Config::getSingleton()->getMCPTileNum()))
      return;

   LOG_PRINT("Creating new MCP object in process %i", m_config.getCurrentProcessNum());

   // FIXME: Can't the MCP look up its network itself in the
   // constructor?
   Tile* mcp_core = m_tile_manager->getTileFromID(m_config.getMCPTileNum());
   LOG_ASSERT_ERROR(mcp_core, "Could not find the MCP's core!");

   Network & mcp_network = *(mcp_core->getNetwork());
   m_mcp = new MCP(mcp_network);

   m_mcp_thread = Thread::create(m_mcp);
   m_mcp_thread->run();
}

void Simulator::endMCP()
{
   if (m_config.getCurrentProcessNum() == m_config.getProcessNumForTile(m_config.getMCPTileNum()))
      m_mcp->finish();
}

bool Simulator::finished()
{
   return m_finished;
}

void Simulator::enablePerformanceModelsInCurrentProcess()
{
   Sim()->startTimer();
   for (UInt32 i = 0; i < Sim()->getConfig()->getNumLocalTiles(); i++)
      Sim()->getTileManager()->getTileFromIndex(i)->enablePerformanceModels();
}

void Simulator::disablePerformanceModelsInCurrentProcess()
{
   Sim()->stopTimer();
   for (UInt32 i = 0; i < Sim()->getConfig()->getNumLocalTiles(); i++)
      Sim()->getTileManager()->getTileFromIndex(i)->disablePerformanceModels();
}

