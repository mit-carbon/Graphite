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

      void initializeCommId(UInt32 comm_id);
      void initializeThread();
      void initializeThread(UInt32 core_id);
      void terminateThread();
      int registerSimMemThread();

      // The following function returns the global ID of the currently running thread
      UInt32 getCurrentCoreID();

      Core *getCurrentCore();
      Core *getCoreFromID(UInt32 id);
      Core *getCoreFromIndex(UInt32 index);

      void outputSummary();

   private:
      UInt32 getCurrentTID();

      Lock *m_maps_lock;

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
