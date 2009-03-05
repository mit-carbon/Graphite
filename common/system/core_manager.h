#ifndef CORE_MANAGER_H
#define CORE_MANAGER_H

#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include "locked_hash.h"

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
      Core *getCoreFromID(core_id_t id);
      Core *getCoreFromIndex(UInt32 index);

      void outputSummary();

   private:
      UInt32 getCurrentTID();

      Lock m_maps_lock;

      // tid_map takes core # to pin thread id
      // core_map takes pin thread id to core # (it's the reverse map)
      UInt32 *tid_map;
      LockedHash tid_to_core_map;
      LockedHash tid_to_core_index_map;

      // Mapping for the simulation threads
      UInt32 *core_to_simthread_tid_map;
      LockedHash simthread_tid_to_core_map;

      std::vector<Core*> m_cores;
};

#endif
