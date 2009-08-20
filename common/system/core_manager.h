#ifndef CORE_MANAGER_H
#define CORE_MANAGER_H

#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include "fixed_types.h"
#include "tls.h"
#include "lock.h"

class Core;
class Lock;

class CoreManager
{
   public:
      CoreManager();
      ~CoreManager();

      void initializeCommId(SInt32 comm_id);
      void initializeThread();
      void initializeThread(core_id_t core_id);
      void terminateThread();
      core_id_t registerSimMemThread();

      core_id_t getCurrentCoreID(); // id of currently active core (or INVALID_CORE_ID)
      core_id_t getCurrentSimThreadCoreID(); // id of core associated with this sim thread (or INVALID_CORE_ID)

      Core *getCurrentCore();
      UInt32 getCurrentCoreIndex();
      Core *getCoreFromID(core_id_t id);
      Core *getCoreFromIndex(UInt32 index);

      void outputSummary(std::ostream &os);

      UInt32 getCoreIndexFromID(core_id_t core_id);

      bool amiUserThread();
      bool amiSimThread();
   private:

      void doInitializeThread(UInt32 core_index);

      UInt32 *tid_map;
      TLS *m_core_tls;
      TLS *m_core_index_tls;
      TLS *m_thread_type_tls;

      enum ThreadType {
          INVALID,
          APP_THREAD,
          SIM_THREAD
      };

      std::vector<bool> m_initialized_cores;
      Lock m_initialized_cores_lock;

      UInt32 m_num_registered_sim_threads;
      Lock m_num_registered_sim_threads_lock;

      std::vector<Core*> m_cores;
};

#endif
