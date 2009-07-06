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
      :
      tid_to_core_map(3*Config::getSingleton()->getNumLocalCores()),
      tid_to_core_index_map(3*Config::getSingleton()->getNumLocalCores()),
      simthread_tid_to_core_map(3*Config::getSingleton()->getNumLocalCores())
{
   LOG_PRINT("Starting CoreManager Constructor.");

   UInt32 num_local_cores = Config::getSingleton()->getNumLocalCores();

   tid_map = new UInt32 [num_local_cores];
   core_to_simthread_tid_map = new UInt32 [num_local_cores];

   UInt32 proc_id = Config::getSingleton()->getCurrentProcessNum();
   const Config::CoreList &local_cores = Config::getSingleton()->getCoreListForProcess(proc_id);

   for (UInt32 i = 0; i < num_local_cores; i++)
   {
      tid_map[i] = UINT_MAX;
      core_to_simthread_tid_map[i] = UINT_MAX;
      m_cores.push_back(new Core(local_cores[i]));
   }

   LOG_PRINT("Finished CoreManager Constructor.");
}

CoreManager::~CoreManager()
{
   for (std::vector<Core *>::iterator i = m_cores.begin(); i != m_cores.end(); i++)
      delete *i;

   delete [] core_to_simthread_tid_map;
   delete [] tid_map;
}

void CoreManager::initializeCommId(SInt32 comm_id)
{
   UInt32 tid = getCurrentTID();
   pair<bool, UInt64> e = tid_to_core_map.find(tid);

   LOG_ASSERT_ERROR(e.first, "initializeCommId: Called without binding thread to a core.");
   core_id_t core_id = e.second;

   UnstructuredBuffer send_buff;
   send_buff << (SInt32)LCP_MESSAGE_COMMID_UPDATE << comm_id << core_id;

   LOG_PRINT("Initializing comm_id: %d to core_id: %d", comm_id, core_id);

   // Broadcast this update to other processes

   e = tid_to_core_index_map.find(tid);
   LOG_ASSERT_ERROR(e.first, "initializeCommId: tid mapped to core, but not to an index?");
   UInt32 idx = (UInt32) e.second;

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
   ScopedLock scoped_maps_lock(m_maps_lock);
   UInt32 tid = getCurrentTID();
   pair<bool, UInt64> e = tid_to_core_map.find(tid);

   LOG_ASSERT_WARNING(e.first == false, "Thread: %d already mapped to core: %lld", tid, e.second);
   const Config::CoreList &core_list = Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum());

   LOG_ASSERT_ERROR(core_list.size() == Config::getSingleton()->getNumLocalCores(),
                    "Core list size different from num local cores? %d != %d", core_list.size(), Config::getSingleton()->getNumLocalCores());

   for (UInt32 i = 0; i < core_list.size(); i++)
   {
      if (tid_map[i] == UINT_MAX)
      {
         core_id_t core_id = core_list[i];
         tid_map[i] = tid;
         tid_to_core_index_map.insert(tid, i);
         tid_to_core_map.insert(tid, core_id);
         LOG_PRINT("Initialize thread : index %d mapped to: thread %d, core_id: %d", i, tid, core_id);
         return;
      }
   }

   LOG_PRINT_ERROR("initializeThread - No free cores out of %d total.", Config::getSingleton()->getNumLocalCores());
}

void CoreManager::initializeThread(core_id_t core_id)
{
   ScopedLock scoped_maps_lock(m_maps_lock);
   UInt32 tid = getCurrentTID();
   pair<bool, UInt64> e = tid_to_core_map.find(tid);

   LOG_ASSERT_ERROR(e.first == false, "Tried to initialize core %d twice.", core_id);

   const Config::CoreList &core_list = Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum());

   LOG_ASSERT_ERROR(core_list.size() == Config::getSingleton()->getNumLocalCores(),
                    "Core list size different from num local cores? %d != %d", core_list.size(), Config::getSingleton()->getNumLocalCores());

   for (UInt32 i = 0; i < core_list.size(); i++)
   {
      core_id_t local_core_id = core_list[i];
      if(local_core_id == core_id)
      {
         if (tid_map[i] == UINT_MAX)
         {
            tid_map[i] = tid;
            tid_to_core_index_map.insert(tid, i);
            tid_to_core_map.insert(tid, core_id);
            LOG_PRINT("Initialize thread : index %d mapped to: thread %d, core_id: %d", i, tid_map[i], core_id);
            return;
         }
         else
         {
            LOG_PRINT_ERROR("initializeThread -- %d/%d already mapped to thread %d", i, Config::getSingleton()->getNumLocalCores(), tid_map[i]);
         }
      }
   }

   LOG_PRINT_ERROR("initializeThread - Requested core %d does not live on process %d.", core_id, Config::getSingleton()->getCurrentProcessNum());
}

void CoreManager::terminateThread()
{
   ScopedLock scoped_maps_lock(m_maps_lock);
   UInt32 tid = getCurrentTID();
   LOG_PRINT("CoreManager::terminating thread: %d", tid);
   pair<bool, UInt64> e = tid_to_core_map.find(tid);

   LOG_ASSERT_WARNING(e.first == true, "Thread: %lld not initialized while terminating.", e.second);

   // If it's not in the tid_to_core_map, well then we don't need to remove it
   if(e.first == false)
       return;

   for (UInt32 i = 0; i < Config::getSingleton()->getNumLocalCores(); i++)
   {
      if (tid_map[i] == tid)
      {
         tid_map[i] = UINT_MAX;
         tid_to_core_index_map.remove(tid);
         tid_to_core_map.remove(tid);
         LOG_PRINT("Terminate thread : removed %lld", e.second);
         return;
      }
   }

   LOG_PRINT_ERROR("terminateThread - Thread tid: %lld not found in list.", e.second);
}

core_id_t CoreManager::getCurrentCoreID()
{
   core_id_t core_id;
   UInt32 tid = getCurrentTID();

   pair<bool, UInt64> e = tid_to_core_map.find(tid);
   core_id = (e.first == false) ? INVALID_CORE_ID : e.second;

   LOG_ASSERT_ERROR(!e.first || core_id < (SInt32)Config::getSingleton()->getTotalCores(), "Illegal core_id value returned by getCurrentCoreID!");

   return core_id;
}

core_id_t CoreManager::getCurrentSimThreadCoreID()
{
   core_id_t core_id;
   UInt32 tid = getCurrentTID();

   pair<bool, UInt64> e = simthread_tid_to_core_map.find(tid);
   core_id = (e.first == false) ? INVALID_CORE_ID : e.second;

   LOG_ASSERT_ERROR(!e.first || core_id < (SInt32)Config::getSingleton()->getTotalCores(), "Illegal core_id value returned by getCurrentCoreID!");

   return core_id;
}

Core *CoreManager::getCurrentCore()
{
   Core *core;
   UInt32 tid = getCurrentTID();

   pair<bool, UInt64> e = tid_to_core_index_map.find(tid);
   core = (e.first == false) ? NULL : m_cores[e.second];

   LOG_ASSERT_ERROR(!e.first || e.second < Config::getSingleton()->getTotalCores(), "Illegal core_id value returned by getCurrentCore!");
   return core;
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

UInt32 CoreManager::getCoreIndexFromTID(UInt32 tid)
{
   pair<bool, UInt64> e = tid_to_core_index_map.find(tid);
   LOG_ASSERT_ERROR(e.first, "getCoreIndexFromTID: couldn't find core for tid: %d", tid);
   return e.second;
}

core_id_t CoreManager::registerSimMemThread()
{
   UInt32 tid = getCurrentTID();

   ScopedLock sl(m_maps_lock);
   pair<bool, UInt64> e = simthread_tid_to_core_map.find(tid);

   // If this thread isn't registered
   if (e.first == false)
   {
      // Search for an unused core to map this simthread thread to
      // one less to account for the MCP
      for (UInt32 i = 0; i < Config::getSingleton()->getNumLocalCores(); i++)
      {
         // Unused slots are set to UINT_MAX
         // FIXME: Use a different constant than UINT_MAX
         if (core_to_simthread_tid_map[i] == UINT_MAX)
         {
            core_to_simthread_tid_map[i] = tid;
            core_id_t core_id = Config::getSingleton()->getCoreListForProcess(Config::getSingleton()->getCurrentProcessNum())[i];
            simthread_tid_to_core_map.insert(tid, core_id);
            return core_id;
         }
      }

      LOG_PRINT("registerSimMemThread - No free cores for thread: %d", tid);
      for (UInt32 j = 0; j < Config::getSingleton()->getNumLocalCores(); j++)
         LOG_PRINT("core_to_simthread_tid_map[%d] = %d", j, core_to_simthread_tid_map[j]);
      LOG_PRINT_ERROR("");
   }
   else
   {
      LOG_PRINT_WARNING("registerSimMemThread - Initialized thread twice");
      // FIXME: I think this is OK
      return simthread_tid_to_core_map.find(tid).second;
   }

   return INVALID_CORE_ID;
}

bool CoreManager::amiSimThread()
{
   UInt32 tid = getCurrentTID();

   ScopedLock sl(m_maps_lock);
   pair<bool, UInt64> e = simthread_tid_to_core_map.find(tid);

   return e.first == true;
}

bool CoreManager::amiUserThread()
{
   UInt32 tid = getCurrentTID();

   ScopedLock sl(m_maps_lock);
   pair<bool, UInt64> e = tid_to_core_map.find(tid);

   return e.first == true;
}


UInt32 CoreManager::getCurrentTID()
{
   return  syscall(__NR_gettid);
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
      LOG_PRINT("Collect from process %p", p);

      const Config::CoreList &cl = cfg->getCoreListForProcess(p);

      for (UInt32 c = 0; c < cl.size(); c++)
      {
         Byte *buf = global_node->recv();

         summaries[cl[c]] = std::string((char*)buf);

         delete [] buf;
      }
   }

   for (UInt32 i = 0; i < summaries.size(); i++)
   {
      LOG_ASSERT_ERROR(!summaries[i].empty(), "Summary %d is empty!", i);
   }
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

   Config *cfg = Config::getSingleton();
   Transport::Node *global_node = Transport::getSingleton()->getGlobalNode();

   for (UInt32 i = 0; i < cfg->getNumLocalCores(); i++)
   {
      LOG_PRINT("Output summary core %i", i);
      std::stringstream ss;
      m_cores[i]->outputSummary(ss);
      global_node->globalSend(0, ss.str().c_str(), ss.str().length()+1);
   }

   if (cfg->getCurrentProcessNum() != 0)
      return;

   std::vector<std::string> summaries(cfg->getTotalCores());
   string formatted;

   gatherSummaries(summaries);
   formatted = formatSummaries(summaries);

   os << formatted;                   
}
