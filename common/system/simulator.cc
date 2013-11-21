#include <cstdlib>
#include <sstream>
#include <iomanip>

#include "simulator.h"
#include "version.h"
#include "log.h"
#include "lcp.h"
#include "mcp.h"
#include "tile.h"
#include "instruction.h"
#include "tile_manager.h"
#include "thread_manager.h"
#include "thread_scheduler.h"
#include "performance_counter_manager.h"
#include "sim_thread_manager.h"
#include "dvfs_manager.h"
#include "clock_skew_management_object.h"
#include "statistics_manager.h"
#include "statistics_thread.h"
#include "contrib/dsent/dsent_contrib.h"
#include "contrib/mcpat/cacti/io.h"

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
   assert(m_singleton);
}

void Simulator::setConfig(config::Config *cfg)
{
   m_config_file = cfg;
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
   : m_mcp(NULL)
   , m_mcp_thread(NULL)
   , m_lcp(NULL)
   , m_lcp_thread(NULL)
   , m_config()
   , m_log(m_config)
   , m_transport(NULL)
   , m_tile_manager(NULL)
   , m_thread_manager(NULL)
   , m_thread_scheduler(NULL)
   , m_performance_counter_manager(NULL)
   , m_sim_thread_manager(NULL)
   , m_clock_skew_management_manager(NULL)
   , m_statistics_manager(NULL)
   , m_statistics_thread(NULL)
   , m_finished(false)
   , m_boot_time(getTime())
   , m_start_time(0)
   , m_stop_time(0)
   , m_shutdown_time(0)
   , m_enabled(false)
{
}

void Simulator::start()
{
   LOG_PRINT("In Simulator ctor.");

   m_config.logTileMap();

   // Get Graphite Home
   char* graphite_home_str = getenv("GRAPHITE_HOME");
   m_graphite_home = (graphite_home_str) ? ((string)graphite_home_str) : ".";
  
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      // Initialize DSENT for network power modeling - create config object
      string dsent_path = m_graphite_home + "/contrib/dsent";
      dsent_contrib::DSENTInterface::allocate(dsent_path, getCfg()->getInt("general/technology_node"));
      dsent_contrib::DSENTInterface::getSingleton()->add_global_tech_overwrite("Temperature",
         getCfg()->getFloat("general/temperature"));

      // Initialize McPAT for core + cache power modeling
      string mcpat_path = m_graphite_home + "/contrib/mcpat";
      McPAT::initializeDatabase(mcpat_path);
   }
  
   m_transport = Transport::create();

   // Initialize the DVFS
   DVFSManager::initializeDVFS();

   m_tile_manager = new TileManager();
   m_thread_manager = new ThreadManager(m_tile_manager);
   m_thread_scheduler = ThreadScheduler::create(m_thread_manager, m_tile_manager);
   m_performance_counter_manager = new PerformanceCounterManager();
   m_sim_thread_manager = new SimThreadManager();
   m_clock_skew_management_manager = ClockSkewManagementManager::create(getCfg()->getString("clock_skew_management/scheme"));
   
   // For periodically measuring statistics
   if (m_config_file->getBool("statistics_trace/enabled"))
   {
      m_statistics_manager = new StatisticsManager();
      m_statistics_thread = new StatisticsThread(m_statistics_manager);
      m_statistics_thread->start();
   }

   startMCP();

   m_sim_thread_manager->spawnSimThreads();

   m_lcp = new LCP();
   m_lcp_thread = Thread::create(m_lcp);
   m_lcp_thread->run();
}

Simulator::~Simulator()
{
   m_shutdown_time = getTime();

   LOG_PRINT("Simulator dtor starting...");

   broadcastFinish();

   endMCP();

   if (m_statistics_thread)
      m_statistics_thread->finish();

   m_lcp->finish();

   m_transport->barrier();

   if (Config::getSingleton()->getCurrentProcessNum() == 0)
   {
      ofstream os(Config::getSingleton()->getOutputFileName().c_str());

      os << "Graphite " << version  << endl << endl;
      os << "Simulation (Host) Timers: " << endl << left
         << setw(35) << "Start Time (in microseconds)" << (m_start_time - m_boot_time) << endl
         << setw(35) << "Stop Time (in microseconds)" << (m_stop_time - m_boot_time) << endl
         << setw(35) << "Shutdown Time (in microseconds)" << (m_shutdown_time - m_boot_time) << endl;

      m_tile_manager->outputSummary(os);
      os.close();
   }
   else
   {
      stringstream temp;
      m_tile_manager->outputSummary(temp);
      assert(temp.str().length() == 0);
   }

   m_sim_thread_manager->quitSimThreads();

   m_transport->barrier();

   delete m_lcp_thread;
   delete m_mcp_thread;
   delete m_lcp;
   delete m_mcp;
   
   // For periodically measuring statistics
   if (m_statistics_thread)
   {
      delete m_statistics_thread;
      delete m_statistics_manager;
   }
  
   // Clock Skew Manager 
   if (m_clock_skew_management_manager)
      delete m_clock_skew_management_manager;

   delete m_sim_thread_manager;
   delete m_performance_counter_manager;
   delete m_thread_manager;
   delete m_thread_scheduler;
   delete m_tile_manager;
   m_tile_manager = NULL;
   delete m_transport;

   // Release DSENT interface object
   if (Config::getSingleton()->getEnablePowerModeling())
      dsent_contrib::DSENTInterface::release();
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

void Simulator::enableModels()
{
   startTimer();
   m_enabled = true;
   for (UInt32 i = 0; i < m_config.getNumLocalTiles(); i++)
      m_tile_manager->getTileFromIndex(i)->enableModels();
}

void Simulator::disableModels()
{
   stopTimer();
   m_enabled = false;
   for (UInt32 i = 0; i < m_config.getNumLocalTiles(); i++)
      m_tile_manager->getTileFromIndex(i)->disableModels();
}

void Simulator::enablePerformanceModelsInCurrentProcess()
{
   Sim()->enableModels();
}

void Simulator::disablePerformanceModelsInCurrentProcess()
{
   Sim()->disableModels();
}
