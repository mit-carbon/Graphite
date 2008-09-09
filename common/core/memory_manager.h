#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <string>
#include <sstream>
#include <iostream>
#include "assert.h"
#include <math.h>
#include "debug.h"

// some forward declarations for cross includes
class Core;

// TODO: this is a hack that is due to the fact that network.h 
// is already included by the time this is handled, so NetPacket is 
// never getting defined. Fine some more elegant way to solve this.
typedef struct NetPacket NetPacket;

#include "core.h"
#include "ocache.h"
#include "dram_directory.h"
#include "cache_directory.h"
#include "dram_directory_entry.h"
#include "cache_directory_entry.h"
#include "network.h"
#include "address_home_lookup.h"
#include "cache_state.h"


// TODO: is there a better way to set up these index constants
//TODO: refactor into a Payload Class/Struct
//
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
#define SH_MEM_ACK_NUM_INTS_SIZE     (2)

// new cstate cache was set to
#define SH_MEM_ACK_IDX_NEW_CSTATE    (0)

// updated base memory address
#define SH_MEM_ACK_IDX_ADDRESS  (1)



extern LEVEL_BASE::KNOB<BOOL> g_knob_simarch_has_shared_mem; 


// TODO: move this into MemoryManager class?
enum shmem_req_t {
  READ, 
  WRITE,
  INVALIDATE,
  NUM_STATES
};

class MemoryManager
{
 private:
  Core *the_core;
  OCache *ocache;
  DramDirectory *dram_dir;
//  CacheDirectory *cache_dir;
  AddressHomeLookup *addr_home_lookup;
  
 public:
	MemoryManager(Core *the_core_arg, OCache *ocache_arg);
	virtual ~MemoryManager();
	bool initiateSharedMemReq(ADDRINT address, UINT32 size, shmem_req_t shmem_req_type);
	void processSharedMemReq(NetPacket req_packet);
	void processUnexpectedSharedMemUpdate(NetPacket update_packet);

	static string sMemReqTypeToString(shmem_req_t type);
// these below functions have been pushed into initiateSharedMemReq (which directly calls ocache->runModel...)
//  bool runDCacheLoadModel(ADDRINT d_addr, UINT32 size);
//  bool runDCacheStoreModel(ADDRINT d_addr, UINT32 size);
  
};

#endif
