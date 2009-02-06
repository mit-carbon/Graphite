#ifndef CORE_MANAGER_H
#define CORE_MANAGER_H

#include <iostream>
#include <fstream>
#include <map>

#include "locked_hash.h"

class Core;
class Lock;

class CoreManager
{
    public:
        CoreManager();
        ~CoreManager();

        void initializeThread(int rank);
        void initializeThreadFree(int *rank);
        int registerSharedMemThread();

        // The following function returns the global ID of the currently running thread
        UInt32 getCurrentCoreID();

        Core *getCurrentCore();
        Core *getCoreFromID(unsigned int id);

        void fini(int code, void *v);

    private:
        Lock *maps_lock;

        // tid_map takes core # to pin thread id
        // core_map takes pin thread id to core # (it's the reverse map)
        UInt32 *tid_map;
        LockedHash tid_to_core_map;
        LockedHash tid_to_core_index_map;

        // Mapping for the shared memory threads
        UInt32 *core_to_shmem_tid_map;
        LockedHash shmem_tid_to_core_map;
        LockedHash shmem_tid_to_core_index_map;

        int prev_rank;

        Core *m_cores;

        UInt32 getCurrentTID();
};

extern CoreManager *g_core_manager;

#endif
