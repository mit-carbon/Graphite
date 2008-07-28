#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <string>
#include <iostream>
#include "assert.h"

#include "core.h"
#include "ocache.h"
#include "dram_directory.h"
#include "cache_directory.h"
#include "network.h"
#include "address_home_lookup.h"



// TODO: is there a better way to set up these index constants

// define indecies/offsets for shared memory requests. indicies assume integer (4 byte) data sizes

// ***** defines for a shared memory update message (there exist two types, expected updates and unexpected updates)

// size of the request payload (in integer data sizes)
#define SH_MEM_REQ_NUM_INTS_SIZE     (3)

// request type (read or write)
#define SH_MEM_REQ_IDX_REQ_TYPE      (0)

// requested base memory address
#define SH_MEM_REQ_IDX_ADDR          (1)

// number of bytes requested (starting at the base memory address)
#define SH_MEM_REQ_IDX_NUM_BYTES_REQ (2)


// ***** defines for a shared memory update message (there exist two types, expected updates and unexpected updates)

// size of the request payload (in integer data sizes)
#define SH_MEM_UPDATE_NUM_INTS_SIZE     (2)

// new cstate to set cache to
#define SH_MEM_UPDATE_IDX_NEW_CSTATE    (0)

// requested base memory address
#define SH_MEM_UPDATE_IDX_ADDRESS  (1)


// ***** defines for a shared memory update message (there exist two types, expected updates and unexpected updates)

// size of the request payload (in integer data sizes)
#define SH_MEM_ACK_NUM_INTS_SIZE     (1)

// new cstate cache was set to
#define SH_MEM_ACK_IDX_NEW_CSTATE    (0)

// updated base memory address
#define SH_MEM_ACK_IDX_ADDRESS  (1)





extern LEVEL_BASE::KNOB<BOOL> g_knob_simarch_has_shared_mem; 

class MemoryManager
{
private:
    Core *the_core;
	OCache *ocache;
	DramDirectory *dram_dir;
	CacheDirectory *cache_dir;
    AddressHomeLookup addr_home_lookup;
	
    enum shmem_req_t {
		READ, 
		WRITE,
		INVALIDATE,
		NUM_STATES
	};
	
public:
	MemoryManager(Core *the_core, OCache *ocache);
	virtual ~MemoryManager();
	bool runDCacheLoadModel(ADDRINT d_addr, UINT32 size);
	bool runDCacheStoreModel(ADDRINT d_addr, UINT32 size);
};


#endif
