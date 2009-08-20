#include <sched.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <limits.h>
#include <algorithm>

#include "core_manager.h"
#include "core.h"
#include "network.h"
#include "cache.h"
#include "config.h"
#include "packetize.h"
#include "message_types.h"

#include "log.h"

using namespace std;

CoreManager::CoreManager()
      : m_core_tls(TLS::create())
      , m_core_index_tls(TLS::create())
      , m_thread_type_tls(TLS::create())
      , m_num_registered_sim_threads(0)
{
   LOG_PRINT("Starting CoreManager Constructor.");

   UInt32 num_local_cores = Config::getSingleton()->getNumLocalCores();

   UInt32 proc_id = Config::getSingleton()->getCurrentProcessNum();
   const Config::CoreList &local_cores = Config::getSingleton()->getCoreListForProcess(proc_id);

   for (UInt32 i = 0; i < num_local_cores; i++)
   {
      m_cores.push_back(new Core(local_cores[i]));
      m_initialized_cores.push_back(false);
   }

   LOG_PRINT("Finished CoreManager Constructor.");
}

CoreManager::~CoreManager()
{
   for (std::vector<Core *>::iterator i = m_cores.begin(); i != m_cores.end(); i++)
      delete *i;

   delete m_core_tls;
   delete m_core_index_tls;
   delete m_thread_type_tls;
}

void CoreManager::initializeCommId(SInt32 comm_id)
{
   core_id_t core_id = getCurrentCoreID();

   UnstructuredBuffer send_buff;
   send_buff << (SInt32)LCP_MESSAGE_COMMID_UPDATE << comm_id << core_id;

   LOG_PRINT("Initializing comm_id: %d to core_id: %d", comm_id, core_id);

   // Broadcast this update to other processes

   UInt32 idx = getCurrentCoreIndex();

   LOG_ASSERT_ERROR(idx < Config::getSingleton()->getNumLocalCores(), "CoreManager got and index [%d] out of range (0-%d).", 
         idx, Config::getSingleton()->getNumLocalCores());

   Network *network = m_cores[idx]->getNetwork();
   Transport::Node *transport = Transport::getSingleton()->getGlobalNode();
   UInt32 num_procs = Config::getSingleton()->getProcessCount();

   for (UInt32 i = 0; i < num_procs; i++)
   {
      transport->globalSend(i,
                            send_buff.getBuffer(),
                            send_buff.size());
   }

   LOG_PRINT("Waiting for replies from LCPs.");

   for (UInt32 i = 0; i < num_procs; i++)
   {
      network->netRecvType(LCP_COMM_ID_UPDATE_REPLY);
      LOG_PRINT("Received reply from proc: %d", i);
   }

   LOG_PRINT("Finished.");
}

void CoreManager::initializeThread()
{
   ScopedLock sl(m_initialized_cores_lock);

   for (core_id_t i = 0; i < (core_id_t)m_initialized_cores.size(); i++)
   {
       if (!m_initialized_cores[i])
       {
           doInitializeThread(i);
           return;
       }
   }

   LOG_PRINT_ERROR("initializeThread - No free cores out of %d total.", Config::getSingleton()->getNumLocalCores());
}

void CoreManager::initializeThread(core_id_t core_id)
{
   ScopedLock sl(m_initialized_cores_lock);

   const Config::CoreList &core_list = Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum());
   LOG_ASSERT_ERROR(core_list.size() == Config::getSingleton()->getNumLocalCores(),
                    "Core list size different from num local cores? %d != %d", core_list.size(), Config::getSingleton()->getNumLocalCores());

   for (UInt32 i = 0; i < core_list.size(); i++)
   {
      core_id_t local_core_id = core_list[i];
      if (local_core_id == core_id)
      {
          if (m_initialized_cores[i])
              LOG_PRINT_ERROR("initializeThread -- %d/%d already mapped", i, Config::getSingleton()->getNumLocalCores());

          doInitializeThread(i);
          return;
      }
   }

   LOG_PRINT_ERROR("initializeThread - Requested core %d does not live on process %d.", core_id, Config::getSingleton()->getCurrentProcessNum());
}

void CoreManager::doInitializeThread(UInt32 core_index)
{
    m_core_tls->set(m_cores[core_index]);
    m_core_index_tls->setInt(core_index);
    m_thread_type_tls->setInt(APP_THREAD);
    m_initialized_cores[core_index] = true;
    LOG_PRINT("Initialize thread : index %d mapped to core: %p", core_index, m_cores[core_index]);
    fprintf(stderr, "InitializeThread %d\n", core_index);
}

void CoreManager::terminateThread()
{
   LOG_ASSERT_WARNING(m_core_tls->get() != NULL, "Thread not initialized while terminating.");

   m_core_tls->set(NULL);
   m_core_index_tls->setInt(-1);
}

core_id_t CoreManager::getCurrentCoreID()
{
   Core *core = getCurrentCore();
   if (!core)
       return INVALID_CORE_ID;
   else
       return core->getId();
}

core_id_t CoreManager::getCurrentSimThreadCoreID()
{
    return getCurrentCoreID();
}

Core *CoreManager::getCurrentCore()
{
    return m_core_tls->getPtr<Core>();
}

UInt32 CoreManager::getCurrentCoreIndex()
{
    UInt32 idx = m_core_index_tls->getInt();
    LOG_ASSERT_ERROR(idx < m_cores.size(), "Invalid core index.");
    return idx;
}

Core *CoreManager::getCoreFromID(core_id_t id)
{
   Core *core = NULL;
   // Look up the index from the core list
   // FIXME: make this more cached
   const Config::CoreList & cores(Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum()));
   UInt32 idx = 0;
   for (Config::CLCI i = cores.begin(); i != cores.end(); i++)
   {
      if (*i == id)
      {
         core = m_cores[idx];
         break;
      }

      idx++;
   }

   LOG_ASSERT_ERROR(!core || idx < Config::getSingleton()->getNumLocalCores(), "Illegal index in getCoreFromID!");

   return core;
}

Core *CoreManager::getCoreFromIndex(UInt32 index)
{
   LOG_ASSERT_ERROR(index < Config::getSingleton()->getNumLocalCores(), "getCoreFromIndex -- invalid index %d", index);

   return m_cores[index];
}

UInt32 CoreManager::getCoreIndexFromID(core_id_t core_id)
{
   // Look up the index from the core list
   // FIXME: make this more cached
   const Config::CoreList & cores(Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum()));
   UInt32 idx = 0;
   for (Config::CLCI i = cores.begin(); i != cores.end(); i++)
   {
      if (*i == core_id)
         return idx;

      idx++;
   }

   LOG_ASSERT_ERROR(false, "Core lookup failed for core id: %d!", core_id);
   return INVALID_CORE_ID;
}

core_id_t CoreManager::registerSimMemThread()
{
    if (getCurrentCore() != NULL)
    {
        LOG_PRINT_ERROR("registerSimMemThread - Initialized thread twice");
        return getCurrentCore()->getId();
    }

    ScopedLock sl(m_num_registered_sim_threads_lock);

    LOG_ASSERT_ERROR(m_num_registered_sim_threads < Config::getSingleton()->getNumLocalCores(),
                     "All threads already registered. %d > %d",
                     m_num_registered_sim_threads+1, Config::getSingleton()->getNumLocalCores());

    Core *core = m_cores.at(
        Config::getSingleton()
        ->getCoreListForProcess(
            Config::getSingleton()->getCurrentProcessNum()
            )
        .at(m_num_registered_sim_threads));

    m_core_tls->set(core);
    m_core_index_tls->setInt(m_num_registered_sim_threads);
    m_thread_type_tls->setInt(SIM_THREAD);

    ++m_num_registered_sim_threads;

    return core->getId();
}

bool CoreManager::amiSimThread()
{
    return m_thread_type_tls->getInt() == SIM_THREAD;
}

bool CoreManager::amiUserThread()
{
    return m_thread_type_tls->getInt() == APP_THREAD;
}

// -- outputSummary
//
// Collect output summaries for all the cores and send them to process
// zero. This process then formats the output to look pretty. Only
// process zero writes to the output stream passed in.

static void gatherSummaries(std::vector<std::string> &summaries)
{
   Config *cfg = Config::getSingleton();
   Transport::Node *global_node = Transport::getSingleton()->getGlobalNode();

   for (UInt32 p = 0; p < cfg->getProcessCount(); p++)
   {
      LOG_PRINT("Collect from process %d", p);

      const Config::CoreList &cl = cfg->getCoreListForProcess(p);

      // signal process to send
      if (p != 0)
         global_node->globalSend(p, &p, sizeof(p));

      // receive summary
      for (UInt32 c = 0; c < cl.size(); c++)
      {
         LOG_PRINT("Collect from core %d", cl[c]);

         Byte *buf;

         buf = global_node->recv();
         assert(*((core_id_t*)buf) == cl[c]);
         delete [] buf;

         buf = global_node->recv();
         summaries[cl[c]] = std::string((char*)buf);
         delete [] buf;
      }
   }

   for (UInt32 i = 0; i < summaries.size(); i++)
   {
      LOG_ASSERT_ERROR(!summaries[i].empty(), "Summary %d is empty!", i);
   }

   LOG_PRINT("Done collecting.");
}

class Table
{
public:
   Table(unsigned int rows,
         unsigned int cols)
      : m_table(rows * cols)
      , m_rows(rows)
      , m_cols(cols)
   { }

   string& operator () (unsigned int r, unsigned int c)
   {
      return at(r,c);
   }
   
   string& at(unsigned int r, unsigned int c)
   {
      assert(r < rows() && c < cols());
      return m_table[ r * m_cols + c ];
   }

   const string& at(unsigned int r, unsigned int c) const
   {
      assert(r < rows() && c < cols());
      return m_table[ r * m_cols + c ];
   }

   std::string flatten() const
   {
      std::vector<unsigned int> col_widths;

      for (unsigned int i = 0; i < cols(); i++)
         col_widths.push_back(0);

      for (unsigned int r = 0; r < rows(); r++)
         for (unsigned int c = 0; c < cols(); c++)
            if (at(r,c).length() > col_widths[c])
               col_widths[c] = at(r,c).length();

      stringstream out;

      for (unsigned int r = 0; r < rows(); r++)
      {
         for (unsigned int c = 0; c < cols(); c++)
         {
            out << at(r,c);
            
            unsigned int padding = col_widths[c] - at(r,c).length();
            out << string(padding, ' ') << " | ";
         }

         out << '\n';
      }

      return out.str();
   }

   unsigned int rows() const
   {
      return m_rows;
   }

   unsigned int cols() const
   {
      return m_cols;
   }

   typedef std::vector<std::string>::size_type size_type;

private:
   std::vector<std::string> m_table;
   unsigned int m_rows;
   unsigned int m_cols;
};

void addRowHeadings(Table &table, const std::vector<std::string> &summaries)
{
   // take row headings from first summary

   const string &sum = summaries[0];
   std::string::size_type pos = 0;

   for (Table::size_type i = 1; i < table.rows(); i++)
   {
      std::string::size_type end = sum.find(':', pos);
      std::string heading = sum.substr(pos, end-pos);
      pos = sum.find('\n', pos) + 1;
      assert(pos != std::string::npos);

      table(i,0) = heading;
   }
}

void addColHeadings(Table &table)
{
   for (Table::size_type i = 0; i < Config::getSingleton()->getApplicationCores(); i++)
   {
      stringstream heading;
      heading << "Core " << i;
      table(0, i+1) = heading.str();
   }

   for (unsigned int i = 0; i < Config::getSingleton()->getProcessCount(); i++)
   {
      unsigned int core_num = Config::getSingleton()->getThreadSpawnerCoreNum(i);
      stringstream heading;
      heading << "TS " << i;
      table(0, core_num + 1) = heading.str();
   }

   table(0, Config::getSingleton()->getMCPCoreNum()+1) = "MCP";
}

void addCoreSummary(Table &table, core_id_t core, const std::string &summary)
{
   std::string::size_type pos = summary.find(':')+1;

   for (Table::size_type i = 1; i < table.rows(); i++)
   {
      std::string::size_type end = summary.find('\n',pos);
      std::string value = summary.substr(pos, end-pos);
      pos = summary.find(':',pos)+1;
      assert(pos != std::string::npos);

      table(i, core+1) = value;
   }
}

std::string formatSummaries(const std::vector<std::string> &summaries)
{
   // assume that each core outputs the same information
   // assume that output is formatted as "label: value"

   // from first summary, find number of rows needed
   unsigned int rows = std::count(summaries[0].begin(), summaries[0].end(), '\n');

   // fill in row headings
   Table table(rows+1, Config::getSingleton()->getTotalCores()+1);

   addRowHeadings(table, summaries);
   addColHeadings(table);

   for (unsigned int i = 0; i < summaries.size(); i++)
   {
      addCoreSummary(table, i, summaries[i]);
   }

   return table.flatten();
}

void CoreManager::outputSummary(std::ostream &os)
{
   LOG_PRINT("Starting CoreManager::outputSummary");

   // Note: Using the global_node only works here because the lcp has
   // finished and therefore is no longer waiting on a receive. This
   // is not the most obvious thing, so maybe there should be a
   // cleaner solution.

   Config *cfg = Config::getSingleton();
   Transport::Node *global_node = Transport::getSingleton()->getGlobalNode();

   // wait for my turn...
   if (cfg->getCurrentProcessNum() != 0)
   {
      Byte *buf = global_node->recv();
      assert(*((UInt32*)buf) == cfg->getCurrentProcessNum());
      delete [] buf;
   }

   // send each summary
   const Config::CoreList &cl = cfg->getCoreListForProcess(cfg->getCurrentProcessNum());

   for (UInt32 i = 0; i < cl.size(); i++)
   {
      LOG_PRINT("Output summary core %i", cl[i]);
      std::stringstream ss;
      m_cores[i]->outputSummary(ss);
      global_node->globalSend(0, &cl[i], sizeof(cl[i]));
      global_node->globalSend(0, ss.str().c_str(), ss.str().length()+1);
   }

   // format (only done on proc 0)
   if (cfg->getCurrentProcessNum() != 0)
      return;

   std::vector<std::string> summaries(cfg->getTotalCores());
   string formatted;

   gatherSummaries(summaries);
   formatted = formatSummaries(summaries);

   os << formatted;                   

   LOG_PRINT("Finished outputSummary");
}
