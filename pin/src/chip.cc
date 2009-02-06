#include <sched.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "chip.h"
#include "sync_client.h"
#include "shared_mem.h"
#include "config.h"

#include "log.h"
#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE CHIP

using namespace std;

Chip::Chip()
    :
        tid_to_core_map(3*g_config->numLocalCores()),
        tid_to_core_index_map(3*g_config->numLocalCores()),
        shmem_tid_to_core_map(3*g_config->numLocalCores()),
        shmem_tid_to_core_index_map(3*g_config->numLocalCores()),
        prev_rank(0) 
{
    InitLock (&maps_lock);

    tid_map = new THREADID [g_config->numLocalCores()];
    core_to_shmem_tid_map = new THREADID [g_config->numLocalCores()];

    m_cores = new Core[g_config->numLocalCores()];

    // Need to subtract 1 for the MCP
    for(UInt32 i = 0; i < g_config->numLocalCores(); i++) 
    {
        tid_map[i] = UINT_MAX;
        core_to_shmem_tid_map[i] = UINT_MAX;
        m_cores[i].coreInit(i, g_config->numLocalCores());
    }

    LOG_PRINT("Finished Chip Constructor.");
}

void Chip::initializeThread(int rank)
{
    THREADID tid = getCurrentTID();

    pair<bool, UINT64> e = tid_to_core_map.find(tid);

    //FIXME: Check to see if two cores try to grab the same rank

    if ( e.first == false ) {
        tid_map[rank] = tid;
        tid_to_core_map.insert( tid, rank );
    }
    else
    {
        cerr << "chipInit Error initializing core twice: " << dec << rank << endl;
    }

}

void Chip::initializeThreadFree(int *rank)
{
    THREADID tid = getCurrentTID();

    GetLock (&maps_lock, 1);

    pair<bool, UINT64> e = tid_to_core_map.find(tid);

    //FIXME: Check to see if two cores try to grab the same rank

    if ( e.first == false ) {
        // Don't allow free initializion of the MCP which claimes the
        // highest core.
        for(unsigned int i = 0; i < g_config->numLocalCores() - 1; i++)
        {
            if (tid_map[i] == UINT_MAX)
            {
                tid_map[i] = tid;    
                tid_to_core_map.insert( tid, i );
                *rank = i;

                // cerr << "chipInit initializing core: " << i << endl;
                ReleaseLock (&maps_lock);
            }
        }

        cerr << "chipInit Error: No Free Cores." << endl;
    }
    else
    {
        //      cerr << "chipInit Error initializing FREE core twice (tid): " << (int)tid << endl;
        //      cerr << "chipInit: Keeping old rank: " << dec << (int)(tid_to_core_map.find(tid).second) << endl;
        //      ASSERT(false, "Error: Core tried to init more than once!\n");
    }

    ReleaseLock (&maps_lock);
}

UInt32 Chip::getCurrentCoreID()
{
    UInt32 id;
    THREADID tid = getCurrentTID();

    pair<bool, UINT64> e = tid_to_core_map.find(tid);
    id = (e.first == false) ? -1 : e.second;

    ASSERT(!e.first || id < g_config->totalCores(), "Illegal rank value returned by chipRank!\n");

    return id;
}

Core *Chip::getCurrentCore()
{
    Core *core;
    THREADID tid = getCurrentTID();

    pair<bool, UINT64> e = tid_to_core_index_map.find(tid);
    core = (e.first == false) ? NULL : &m_cores[e.second];

    ASSERT(!e.first || e.second < g_config->totalCores(), "Illegal rank value returned by chipRank!\n");
    return core;
}

Core *Chip::getCoreFromID(unsigned int id)
{
    ASSERT(id < g_config->totalCores(), "Illegal core id request from getCoreFromID()!\n");

    Core *core = NULL;
    // Look up the index from the core list
    // FIXME: make this more cached
    const Config::CoreList & cores (g_config->getCoreListForProcess(g_config->myProcNum()));
    UInt32 idx = 0;
    for(Config::CLCI i = cores.begin(); i != cores.end(); i++)
    {
        if(*i == id)
        {
            core = &m_cores[idx];
            break;
        }

        idx++;
    }

    ASSERT(!core || idx < g_config->numLocalCores(), "Illegal rank value returned by chipRank!\n");
    return core;
}

void Chip::fini(int code, void *v)
{
    LOG_PRINT("Starting Chip::fini");

    ofstream out( g_knob_output_file.Value().c_str() );

    for(UInt32 i = 0; i < g_config->numLocalCores(); i++)
    {
        LOG_PRINT("Output summary core %i", i);

        out << "*** Core[" << i << "] summary ***" << endl;
        m_cores[i].fini(code, v, out); 
        out << endl;
    }

    out.close();

    LOG_PRINT("Finish chip::fini");
}

int Chip::registerSharedMemThread()
{
    // FIXME: I need to lock because there is a race condition
    THREADID tid = getCurrentTID();

    GetLock (&maps_lock, 1);
    pair<bool, UINT64> e = shmem_tid_to_core_map.find(tid);

    // If this thread isn't registered
    if ( e.first == false ) {

        // Search for an unused core to map this shmem thread to
        // one less to account for the MCP
        for(UInt32 i = 0; i < g_config->numLocalCores(); i++)
        {
            // Unused slots are set to UINT_MAX
            // FIXME: Use a different constant than UINT_MAX
            if(core_to_shmem_tid_map[i] == UINT_MAX)
            {
                core_to_shmem_tid_map[i] = tid;    
                shmem_tid_to_core_map.insert( tid, i );
                ReleaseLock (&maps_lock);
                return g_config->getCoreListForProcess(g_config->myProcNum())[i];
            }
        }

        cerr << "chipInit Error: No Free Cores." << endl;
    }
    else
    {
        cerr << "Initialized shared mem thread twice -- id: " << tid << endl;
        ReleaseLock (&maps_lock);
        // FIXME: I think this is OK
        return shmem_tid_to_core_map.find(tid).second;
    }

    ReleaseLock (&maps_lock);
    return -1;
}


THREADID Chip::getCurrentTID()
{
    return  syscall( __NR_gettid );
}

