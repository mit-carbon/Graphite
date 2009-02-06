// Jonathan Eastep and Harshad Kasture
//

#ifndef CHIP_H
#define CHIP_H

#include <iostream>
#include <fstream>
#include <map>

#include "pin.H"
#include "capi.h"
#include "core.h"
#include "ocache.h"
#include "dram_directory_entry.h"
#include "cache_state.h"
#include "address_home_lookup.h"
#include "perfmdl.h"
#include "locked_hash.h"
#include "syscall_model.h"
#include "mcp.h"

extern LEVEL_BASE::KNOB<string> g_knob_output_file;

class Chip 
{
    public:
        Chip();

        void initializeThread(int rank);
        void initializeThreadFree(int *rank);
        int registerSharedMemThread();

        // The following function returns the global ID of the currently running thread
        UInt32 getCurrentCoreID();

        Core *getCurrentCore();
        Core *getCoreFromID(unsigned int id);

        void fini(int code, void *v);

    private:
        PIN_LOCK maps_lock;

        // tid_map takes core # to pin thread id
        // core_map takes pin thread id to core # (it's the reverse map)
        THREADID *tid_map;
        LockedHash tid_to_core_map;
        LockedHash tid_to_core_index_map;

        // Mapping for the shared memory threads
        THREADID *core_to_shmem_tid_map;
        LockedHash shmem_tid_to_core_map;
        LockedHash shmem_tid_to_core_index_map;

        int prev_rank;

        Core *m_cores;

        THREADID getCurrentTID();
};

#endif
