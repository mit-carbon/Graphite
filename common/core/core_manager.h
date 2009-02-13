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

        void initializeThread(UInt32 core_id);
        void initializeThreadFree(int *core_id);
        int registerSharedMemThread();

        // The following function returns the global ID of the currently running thread
        UInt32 getCurrentCoreID();

        Core *getCurrentCore();
        Core *getCoreFromID(unsigned int id);

        void outputSummary();

    private:
        UInt32 getCurrentTID();

        Lock *m_maps_lock;

        // tid_map takes core # to pin thread id
        // core_map takes pin thread id to core # (it's the reverse map)
        UInt32 *tid_map;
        LockedHash tid_to_core_map;
        LockedHash tid_to_core_index_map;

        // Mapping for the shared memory threads
        UInt32 *core_to_shmem_tid_map;
        LockedHash shmem_tid_to_core_map;
        LockedHash shmem_tid_to_core_index_map;

        std::vector<Core*> m_cores;
};

extern CoreManager *g_core_manager;

#endif
