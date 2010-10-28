#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <sstream>
#include "capi.h"

#include "cache_state.h"
#include "dram_directory_entry.h"
#include "tile.h"
#include "chip.h"
#include "pin.H"

using namespace std;

pthread_mutex_t write_lock;
UINT32* global_array_ptr;

#define DEBUG 1
#define TEST_MEMORY_SIZE (pow(2,30))

#define STATE_COUNT 13
#define TEST_COUNT 52

//count the number of tests performed
//output it for each test
int test_count = 0;
int tests_passed = 0;
int tests_failed = 0;

#ifdef DEBUG
pthread_mutex_t lock;
#endif

#define CORE_COUNT 2

enum operation_t
{
   CORE_0_READ_OP,
   CORE_0_WRITE_OP,
   CORE_1_READ_OP,
   CORE_1_WRITE_OP,
   OPERATION_COUNT
};

enum memory_access_t
{
   INIT,
   CORE1_WRITE_10,
   CORE0_WRITE_00,
   CORE0_READ_01,
   CORE0_WRITE_10,
   CORE0_READ_11,
   NUM_MEMORY_ACCESS_TYPES
};

typedef struct
{
   IntPtr address;
   UINT32 dram_addr_home_id;
   DramDirectoryEntry::dstate_t dstate;
   CacheState::cstate_t cstate0;
   CacheState::cstate_t cstate1;
   vector<UINT32> sharers_list;
   char *d_data;
   char *c_data;
} MemoryState;

MemoryState data[2][2][NUM_MEMORY_ACCESS_TYPES];



/* The final memory state for every test is stored
 * in a number of 'fini_vector' arrays.  It is important
 * to know which tests correspond to which core.
 * therefore, for 52 tests, the first 26 correspond
 * to core#0, and the last 26 correspond to core#1
 */
// Responsible for addresses
IntPtr dram0_address[2];
IntPtr dram1_address[2];

char* dramBlock00;
char* dramBlock01;
char* dramBlock10;
char* dramBlock11;

char* cacheBlock00;
char* cacheBlock10;
char* cacheBlock00_10;

UINT32 cacheBlockSize;

// Function executed by each thread
// each thread "starts" here
void* starter_function(void *threadid);

void awesome_test_suite_msi(int tid);

void initialize_test_parameters();

int main(int argc, char* argv[])  // main begins
{

   if (argc != 2)
   {
      cerr << "[Usage]: ./test_evic <logCacheBlockSize>\n";
   }
   // Declare threads and related variables

   // 2 important Simulator variables are initialized here
   UINT32 logCacheBlockSize;

   logCacheBlockSize = atoi(argv[1]);
   cacheBlockSize = 1 << logCacheBlockSize;

   pthread_t threads[2];
   pthread_attr_t attr;

#ifdef DEBUG
   cerr << "This is the function main()" << endl;
#endif
   // Initialize global variables
   global_array_ptr = new UINT32[(UINT64) TEST_MEMORY_SIZE/ (sizeof(UINT32))];
   cerr << "What is the Number of Elements that are being made? = " << (((int) TEST_MEMORY_SIZE/(sizeof(UINT32)))) << endl;
// global_array_ptr = new UINT64[BAJILLION];

#ifdef DEBUG
   cerr << "Initializing thread structures" << endl << endl;
   pthread_mutex_init(&lock, NULL);
#endif

   // Initialize threads and related variables
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   pthread_mutex_init(&write_lock, NULL);

   initialize_test_parameters();

#ifdef DEBUG
   cerr << "Spawning threads" << endl << endl;
#endif

   pthread_create(&threads[0], &attr, starter_function, (void *) 0);
   pthread_create(&threads[1], &attr, starter_function, (void *) 1);

#ifdef DEBUG
   cerr << "Waiting to join" << endl << endl;
#endif

   // Wait for all threads to complete
   // while(1);
   pthread_join(threads[0], NULL);
   pthread_join(threads[1], NULL);

#ifdef DEBUG
   cerr << "End of execution" << endl << endl;
#endif

   // Join the shared memory threads
   return 0;
} // main ends

//spawned threads run this function
void* starter_function(void *threadid)
{
   int tid;
   CAPI_Initialize_FreeRank(&tid);

#ifdef DEBUG
//   pthread_mutex_lock(&lock);
// cerr << "executing do_nothing function: " << tid << endl << endl;
//   pthread_mutex_unlock(&lock);
#endif

   if (tid==0)
   {
//  cerr << "Executing awesome test suite Tile # 0 " << endl;
      awesome_test_suite_msi(tid);
//  cerr << "FInished Executing awesome test suite  Tile #0" << endl;
   }
   else
   {
//  cerr << "Executing awesome test suite Tile #1 " << endl;
      awesome_test_suite_msi(tid);
//  cerr << "FInished Executing awesome test suite  Tile #1" << endl;
   }
   // CAPI_Finish(tid);
   pthread_exit(NULL);
}

void BARRIER_DUAL_CORE(int tid)
{
   //this is a stupid barrier just for the test purposes
   int payload;

   // cerr << "BARRIER DUAL CORE for ID(" << tid << ")" << endl;
   if (tid==0)
   {
      CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &payload, sizeof(int));
      // cerr << "[0]: Start CAPI_Receive from [1]\n";
      CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &payload, sizeof(int));
      // cerr << "[0]: End CAPI_Receive from [1]\n";
   }
   else
   {
      CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &payload, sizeof(int));
      // cerr << "[1]: Start CAPI_Receive from [0]\n";
      CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &payload, sizeof(int));
      // cerr << "[1]: End CAPI_Receive from [0]\n";
   }

}

void SET_INITIAL_MEM_CONDITIONS(IntPtr address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data)
{

   CAPI_debugSetMemState(address, dram_address_home_id, dstate, cstate0, cstate1, sharers_list, d_data, c_data);
}

bool ASSERT_MEMORY_STATE(IntPtr address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data, string error_code)
{
   if (CAPI_debugAssertMemState(address, dram_address_home_id, dstate, cstate0, cstate1, sharers_list, d_data, c_data, "Special Test Cases", error_code) == 1)
   {
      return true;
   }
   else
   {
      return false;
   }
}

/*
IntPtr getAddressOnCore (UINT32 coreId, UINT32 *global_array_ptr) {

 // This is a big big hack
 // Uses 'logDRAMContiguousBlockSize'
 // Lets assume that the address is always cache block aligned for now
 IntPtr start_addr = (IntPtr) global_array_ptr;
 cerr << "test.cc :: start_addr = 0x" << hex << start_addr << endl;
 cerr << "Wanted Address = 0x" << hex << (coreId << logDRAMContiguousBlockSize) << endl;
 // assert (start_addr < (coreId << logDRAMContiguousBlockSize));
 if (start_addr < (coreId << logDRAMContiguousBlockSize))
  return ( (IntPtr) ((coreId << logDRAMContiguousBlockSize) - start_addr) );
 else
   return (0);
}
*/

void initialize_test_parameters()
{
   // UINT32 number_of_elements = ((UINT32) TEST_MEMORY_SIZE ) / sizeof(UINT32);

   /*
   * loop through different operations
   */

   // Get the right addresses and the corresponding home nodes here. After we get the home nodes, add it to the address_vector structure
   // Construct the address_vector
   // Arguments:
   //  tid: thread ID. This will also give me my core ID
   //  This will be a replaceable routine again. This will call into the simulator code to get an address on a particular home node
   //
   // The MemoryState structure gives the state of the memory system (DRAM directory + cache) for a particular address. We operate at the granularity of cache blocks here
   // since we want to test the directory based cache coherence system
   // I must be able to easily modify the code to test evictions and all other complex scenarios that can occur
   // Address:


   // CAPI_getAddress(coreId) : will get an address homed at core with ID 'coreId'

   dram0_address[0] = (IntPtr) &global_array_ptr[0];
   dram0_address[1] = (IntPtr) &global_array_ptr[1];
   dram1_address[0] = (IntPtr) &global_array_ptr[2];
   dram1_address[1] = (IntPtr) &global_array_ptr[3];

   // dram0_address is aliased to an address homed on core '0'
   // dram1_address is aliased to an address homed on core '1'

   CAPI_alias(dram0_address[0], DRAM_0, 0);  // First cache block in dram0, second cache block also on dram0
   CAPI_alias(dram0_address[1], DRAM_0, 1);
   CAPI_alias(dram1_address[0], DRAM_1, 0);
   CAPI_alias(dram1_address[1], DRAM_1, 1);

   /*preset sharer_list arrays for the four possible states
    * we could have a sharers list:
    * {}, {0}, {1}, {01}
    */
   vector<UINT32> sharers_list_null;
   vector<UINT32> sharers_list_0;
   vector<UINT32> sharers_list_1;
   vector<UINT32> sharers_list_0_1;
   sharers_list_0.push_back(0);
   sharers_list_1.push_back(1);
   sharers_list_0_1.push_back(1);
   sharers_list_0_1.push_back(0);

   // Tile 1 reads dram1_address[0]
   // Tile 0 writes dram0_address[0]
   // Tile 1 reads dram0_address[1]
   // Tile 1 writes dram1_address[0]

   // Initial, all blocks are uncached


   dramBlock00 = new char[cacheBlockSize];
   dramBlock01 = new char[cacheBlockSize];
   dramBlock10 = new char[cacheBlockSize];
   dramBlock11 = new char[cacheBlockSize];

   cacheBlock00 = new char[cacheBlockSize];
   cacheBlock10 = new char[cacheBlockSize];
   cacheBlock00_10 = new char[cacheBlockSize];

   memset(dramBlock00, 'a', cacheBlockSize);
   memset(dramBlock01, 'b', cacheBlockSize);
   memset(dramBlock10, 'c', cacheBlockSize);
   memset(dramBlock11, 'd', cacheBlockSize);

   memset(cacheBlock00, 'a', cacheBlockSize);
   memset(cacheBlock10, 'c', cacheBlockSize);
   memset(cacheBlock00_10, 'c', cacheBlockSize);

   for (UINT32 i = 0; i < sizeof(UINT32); i++)
   {
      cacheBlock00[i] = 'A';
      cacheBlock10[i] = 'C';
      cacheBlock00_10[i] = 'A';
   }

   // data[0][0] - OP 0
   data[0][0][INIT].address = dram0_address[0];
   data[0][0][INIT].dram_addr_home_id = 0;
   data[0][0][INIT].dstate = DramDirectoryEntry::UNCACHED;
   data[0][0][INIT].cstate0 = CacheState::INVALID;
   data[0][0][INIT].cstate1 = CacheState::INVALID;
   data[0][0][INIT].sharers_list = sharers_list_null;
   data[0][0][INIT].d_data = dramBlock00;
   data[0][0][INIT].c_data = NULL;

   // data[0][1] - OP 0
   data[0][1][INIT].address = dram0_address[1];
   data[0][1][INIT].dram_addr_home_id = 0;
   data[0][1][INIT].dstate = DramDirectoryEntry::UNCACHED;
   data[0][1][INIT].cstate0 = CacheState::INVALID;
   data[0][1][INIT].cstate1 = CacheState::INVALID;
   data[0][1][INIT].sharers_list = sharers_list_null;
   data[0][1][INIT].d_data = dramBlock01;
   data[0][1][INIT].c_data = NULL;

   // data[0][1] - OP 0
   data[1][0][INIT].address = dram1_address[0];
   data[1][0][INIT].dram_addr_home_id = 1;
   data[1][0][INIT].dstate = DramDirectoryEntry::UNCACHED;
   data[1][0][INIT].cstate0 = CacheState::INVALID;
   data[1][0][INIT].cstate1 = CacheState::INVALID;
   data[1][0][INIT].sharers_list = sharers_list_null;
   data[1][0][INIT].d_data = dramBlock10;
   data[1][0][INIT].c_data = NULL;

   // data[0][1] - OP 0
   data[1][1][INIT].address = dram1_address[1];
   data[1][1][INIT].dram_addr_home_id = 1;
   data[1][1][INIT].dstate = DramDirectoryEntry::UNCACHED;
   data[1][1][INIT].cstate0 = CacheState::INVALID;
   data[1][1][INIT].cstate1 = CacheState::INVALID;
   data[1][1][INIT].sharers_list = sharers_list_null;
   data[1][1][INIT].d_data = dramBlock11;
   data[1][1][INIT].c_data = NULL;

   // After Tile 1 writes data[1][0]

   // data[0][0] - OP 0
   data[0][0][CORE1_WRITE_10].address = dram0_address[0];
   data[0][0][CORE1_WRITE_10].dram_addr_home_id = 0;
   data[0][0][CORE1_WRITE_10].dstate = DramDirectoryEntry::UNCACHED;
   data[0][0][CORE1_WRITE_10].cstate0 = CacheState::INVALID;
   data[0][0][CORE1_WRITE_10].cstate1 = CacheState::INVALID;
   data[0][0][CORE1_WRITE_10].sharers_list = sharers_list_null;
   data[0][0][CORE1_WRITE_10].d_data = dramBlock00;
   data[0][0][CORE1_WRITE_10].c_data = NULL;

   // data[0][1] - OP 0
   data[0][1][CORE1_WRITE_10].address = dram0_address[1];
   data[0][1][CORE1_WRITE_10].dram_addr_home_id = 0;
   data[0][1][CORE1_WRITE_10].dstate = DramDirectoryEntry::UNCACHED;
   data[0][1][CORE1_WRITE_10].cstate0 = CacheState::INVALID;
   data[0][1][CORE1_WRITE_10].cstate1 = CacheState::INVALID;
   data[0][1][CORE1_WRITE_10].sharers_list = sharers_list_null;
   data[0][1][CORE1_WRITE_10].d_data = dramBlock01;
   data[0][1][CORE1_WRITE_10].c_data = NULL;

   // data[0][1] - OP 0
   data[1][0][CORE1_WRITE_10].address = dram1_address[0];
   data[1][0][CORE1_WRITE_10].dram_addr_home_id = 1;
   data[1][0][CORE1_WRITE_10].dstate = DramDirectoryEntry::EXCLUSIVE;
   data[1][0][CORE1_WRITE_10].cstate0 = CacheState::INVALID;
   data[1][0][CORE1_WRITE_10].cstate1 = CacheState::EXCLUSIVE;
   data[1][0][CORE1_WRITE_10].sharers_list = sharers_list_1;
   data[1][0][CORE1_WRITE_10].d_data = dramBlock10;
   data[1][0][CORE1_WRITE_10].c_data = cacheBlock10;

   // data[0][1] - OP 0
   data[1][1][CORE1_WRITE_10].address = dram1_address[1];
   data[1][1][CORE1_WRITE_10].dram_addr_home_id = 1;
   data[1][1][CORE1_WRITE_10].dstate = DramDirectoryEntry::UNCACHED;
   data[1][1][CORE1_WRITE_10].cstate0 = CacheState::INVALID;
   data[1][1][CORE1_WRITE_10].cstate1 = CacheState::INVALID;
   data[1][1][CORE1_WRITE_10].sharers_list = sharers_list_null;
   data[1][1][CORE1_WRITE_10].d_data = dramBlock11;
   data[1][1][CORE1_WRITE_10].c_data = NULL;


   // After Tile 0 writes data[0][0]
   // data[0][0] - OP 0
   data[0][0][CORE0_WRITE_00].address = dram0_address[0];
   data[0][0][CORE0_WRITE_00].dram_addr_home_id = 0;
   data[0][0][CORE0_WRITE_00].dstate = DramDirectoryEntry::EXCLUSIVE;
   data[0][0][CORE0_WRITE_00].cstate0 = CacheState::EXCLUSIVE;
   data[0][0][CORE0_WRITE_00].cstate1 = CacheState::INVALID;
   data[0][0][CORE0_WRITE_00].sharers_list = sharers_list_0;
   data[0][0][CORE0_WRITE_00].d_data = dramBlock00;
   data[0][0][CORE0_WRITE_00].c_data = cacheBlock00;

   // data[0][1] - OP 0
   data[0][1][CORE0_WRITE_00].address = dram0_address[1];
   data[0][1][CORE0_WRITE_00].dram_addr_home_id = 0;
   data[0][1][CORE0_WRITE_00].dstate = DramDirectoryEntry::UNCACHED;
   data[0][1][CORE0_WRITE_00].cstate0 = CacheState::INVALID;
   data[0][1][CORE0_WRITE_00].cstate1 = CacheState::INVALID;
   data[0][1][CORE0_WRITE_00].sharers_list = sharers_list_null;
   data[0][1][CORE0_WRITE_00].d_data = dramBlock01;
   data[0][1][CORE0_WRITE_00].c_data = NULL;

   // data[0][1] - OP 0
   data[1][0][CORE0_WRITE_00].address = dram1_address[0];
   data[1][0][CORE0_WRITE_00].dram_addr_home_id = 1;
   data[1][0][CORE0_WRITE_00].dstate = DramDirectoryEntry::EXCLUSIVE;
   data[1][0][CORE0_WRITE_00].cstate0 = CacheState::INVALID;
   data[1][0][CORE0_WRITE_00].cstate1 = CacheState::EXCLUSIVE;
   data[1][0][CORE0_WRITE_00].sharers_list = sharers_list_1;
   data[1][0][CORE0_WRITE_00].d_data = dramBlock10;
   data[1][0][CORE0_WRITE_00].c_data = cacheBlock10;

   // data[0][1] - OP 0
   data[1][1][CORE0_WRITE_00].address = dram1_address[1];
   data[1][1][CORE0_WRITE_00].dram_addr_home_id = 1;
   data[1][1][CORE0_WRITE_00].dstate = DramDirectoryEntry::UNCACHED;
   data[1][1][CORE0_WRITE_00].cstate0 = CacheState::INVALID;
   data[1][1][CORE0_WRITE_00].cstate1 = CacheState::INVALID;
   data[1][1][CORE0_WRITE_00].sharers_list = sharers_list_null;
   data[1][1][CORE0_WRITE_00].d_data = dramBlock11;
   data[1][1][CORE0_WRITE_00].c_data = NULL;


   // After Tile 0 reads data[0][1]
   // data[0][0] - OP 0
   data[0][0][CORE0_READ_01].address = dram0_address[0];
   data[0][0][CORE0_READ_01].dram_addr_home_id = 0;
   data[0][0][CORE0_READ_01].dstate = DramDirectoryEntry::EXCLUSIVE;
   data[0][0][CORE0_READ_01].cstate0 = CacheState::EXCLUSIVE;
   data[0][0][CORE0_READ_01].cstate1 = CacheState::INVALID;
   data[0][0][CORE0_READ_01].sharers_list = sharers_list_0;
   data[0][0][CORE0_READ_01].d_data = dramBlock00;
   data[0][0][CORE0_READ_01].c_data = cacheBlock00;

   // data[0][1] - OP 0
   data[0][1][CORE0_READ_01].address = dram0_address[1];
   data[0][1][CORE0_READ_01].dram_addr_home_id = 0;
   data[0][1][CORE0_READ_01].dstate = DramDirectoryEntry::SHARED;
   data[0][1][CORE0_READ_01].cstate0 = CacheState::SHARED;
   data[0][1][CORE0_READ_01].cstate1 = CacheState::INVALID;
   data[0][1][CORE0_READ_01].sharers_list = sharers_list_0;
   data[0][1][CORE0_READ_01].d_data = dramBlock01;
   data[0][1][CORE0_READ_01].c_data = dramBlock01;

   // data[0][1] - OP 0
   data[1][0][CORE0_READ_01].address = dram1_address[0];
   data[1][0][CORE0_READ_01].dram_addr_home_id = 1;
   data[1][0][CORE0_READ_01].dstate = DramDirectoryEntry::EXCLUSIVE;
   data[1][0][CORE0_READ_01].cstate0 = CacheState::INVALID;
   data[1][0][CORE0_READ_01].cstate1 = CacheState::EXCLUSIVE;
   data[1][0][CORE0_READ_01].sharers_list = sharers_list_1;
   data[1][0][CORE0_READ_01].d_data = dramBlock10;
   data[1][0][CORE0_READ_01].c_data = cacheBlock10;

   // data[0][1] - OP 0
   data[1][1][CORE0_READ_01].address = dram1_address[1];
   data[1][1][CORE0_READ_01].dram_addr_home_id = 1;
   data[1][1][CORE0_READ_01].dstate = DramDirectoryEntry::UNCACHED;
   data[1][1][CORE0_READ_01].cstate0 = CacheState::INVALID;
   data[1][1][CORE0_READ_01].cstate1 = CacheState::INVALID;
   data[1][1][CORE0_READ_01].sharers_list = sharers_list_null;
   data[1][1][CORE0_READ_01].d_data = dramBlock11;
   data[1][1][CORE0_READ_01].c_data = NULL;


   // After Tile 0 writes data[1][0] - Eviction of d[0][0]
   // data[0][0] - OP 0
   data[0][0][CORE0_WRITE_10].address = dram0_address[0];
   data[0][0][CORE0_WRITE_10].dram_addr_home_id = 0;
   data[0][0][CORE0_WRITE_10].dstate = DramDirectoryEntry::UNCACHED;
   data[0][0][CORE0_WRITE_10].cstate0 = CacheState::INVALID;
   data[0][0][CORE0_WRITE_10].cstate1 = CacheState::INVALID;
   data[0][0][CORE0_WRITE_10].sharers_list = sharers_list_null;
   data[0][0][CORE0_WRITE_10].d_data = cacheBlock00;
   data[0][0][CORE0_WRITE_10].c_data = NULL;

   // data[0][1] - OP 0
   data[0][1][CORE0_WRITE_10].address = dram0_address[1];
   data[0][1][CORE0_WRITE_10].dram_addr_home_id = 0;
   data[0][1][CORE0_WRITE_10].dstate = DramDirectoryEntry::SHARED;
   data[0][1][CORE0_WRITE_10].cstate0 = CacheState::SHARED;
   data[0][1][CORE0_WRITE_10].cstate1 = CacheState::INVALID;
   data[0][1][CORE0_WRITE_10].sharers_list = sharers_list_0;
   data[0][1][CORE0_WRITE_10].d_data = dramBlock01;
   data[0][1][CORE0_WRITE_10].c_data = dramBlock01;

   // data[0][1] - OP 0
   data[1][0][CORE0_WRITE_10].address = dram1_address[0];
   data[1][0][CORE0_WRITE_10].dram_addr_home_id = 1;
   data[1][0][CORE0_WRITE_10].dstate = DramDirectoryEntry::EXCLUSIVE;
   data[1][0][CORE0_WRITE_10].cstate0 = CacheState::EXCLUSIVE;
   data[1][0][CORE0_WRITE_10].cstate1 = CacheState::INVALID;
   data[1][0][CORE0_WRITE_10].sharers_list = sharers_list_0;
   data[1][0][CORE0_WRITE_10].d_data = cacheBlock10;
   data[1][0][CORE0_WRITE_10].c_data = cacheBlock10;

   // data[0][1] - OP 0
   data[1][1][CORE0_WRITE_10].address = dram1_address[1];
   data[1][1][CORE0_WRITE_10].dram_addr_home_id = 1;
   data[1][1][CORE0_WRITE_10].dstate = DramDirectoryEntry::UNCACHED;
   data[1][1][CORE0_WRITE_10].cstate0 = CacheState::INVALID;
   data[1][1][CORE0_WRITE_10].cstate1 = CacheState::INVALID;
   data[1][1][CORE0_WRITE_10].sharers_list = sharers_list_null;
   data[1][1][CORE0_WRITE_10].d_data = dramBlock11;
   data[1][1][CORE0_WRITE_10].c_data = NULL;

   // Tile 0 reads data[1][1] - Eviction of d[0][1]
   // data[0][0] - OP 0
   data[0][0][CORE0_READ_11].address = dram0_address[0];
   data[0][0][CORE0_READ_11].dram_addr_home_id = 0;
   data[0][0][CORE0_READ_11].dstate = DramDirectoryEntry::UNCACHED;
   data[0][0][CORE0_READ_11].cstate0 = CacheState::INVALID;
   data[0][0][CORE0_READ_11].cstate1 = CacheState::INVALID;
   data[0][0][CORE0_READ_11].sharers_list = sharers_list_null;
   data[0][0][CORE0_READ_11].d_data = cacheBlock00;
   data[0][0][CORE0_READ_11].c_data = NULL;

   // data[0][1] - OP 0
   data[0][1][CORE0_READ_11].address = dram0_address[1];
   data[0][1][CORE0_READ_11].dram_addr_home_id = 0;
   data[0][1][CORE0_READ_11].dstate = DramDirectoryEntry::UNCACHED;
   data[0][1][CORE0_READ_11].cstate0 = CacheState::INVALID;
   data[0][1][CORE0_READ_11].cstate1 = CacheState::INVALID;
   data[0][1][CORE0_READ_11].sharers_list = sharers_list_null;
   data[0][1][CORE0_READ_11].d_data = dramBlock01;
   data[0][1][CORE0_READ_11].c_data = NULL;

   // data[0][1] - OP 0
   data[1][0][CORE0_READ_11].address = dram1_address[0];
   data[1][0][CORE0_READ_11].dram_addr_home_id = 1;
   data[1][0][CORE0_READ_11].dstate = DramDirectoryEntry::EXCLUSIVE;
   data[1][0][CORE0_READ_11].cstate0 = CacheState::EXCLUSIVE;
   data[1][0][CORE0_READ_11].cstate1 = CacheState::INVALID;
   data[1][0][CORE0_READ_11].sharers_list = sharers_list_0;
   data[1][0][CORE0_READ_11].d_data = cacheBlock10;
   data[1][0][CORE0_READ_11].c_data = cacheBlock10;

   // data[0][1] - OP 0
   data[1][1][CORE0_READ_11].address = dram1_address[1];
   data[1][1][CORE0_READ_11].dram_addr_home_id = 1;
   data[1][1][CORE0_READ_11].dstate = DramDirectoryEntry::SHARED;
   data[1][1][CORE0_READ_11].cstate0 = CacheState::SHARED;
   data[1][1][CORE0_READ_11].cstate1 = CacheState::INVALID;
   data[1][1][CORE0_READ_11].sharers_list = sharers_list_0;
   data[1][1][CORE0_READ_11].d_data = dramBlock11;
   data[1][1][CORE0_READ_11].c_data = dramBlock11;


   /******************************************************/
   /******** FINISHED SETTING FINAL MEMORY STATES ********/
   /******************************************************/
}

string operationToString(operation_t op)
{
   switch (op)
   {
   case CORE_0_READ_OP:
      return "CORE#0 -READ-";
   case CORE_1_READ_OP:
      return "CORE#1 -READ-";
   case CORE_0_WRITE_OP:
      return "CORE#0 -WRITE-";
   case CORE_1_WRITE_OP:
      return "CORE#1 -WRITE-";
   default:
      return "ERROR IN OP TO STRING";
   }
}

//hand in the thread id#, or tid
void awesome_test_suite_msi(int tid)
{

   UINT32 write_value = 0xaaaaaaaa; //dummy load/write variable

   /********************************************/
   /********************************************/
   /********************************************/

   if (tid == 0)
   {
      for (UINT32 i = 0; i < 2; i++)
      {
         for (UINT32 j = 0; j < 2; j++)
         {
            // cerr << "Test_1.cc: d_data = 0x" << hex << (UINT32) data[i][j][INIT].d_data << endl;
            assert(data[i][j][INIT].d_data != NULL);
            SET_INITIAL_MEM_CONDITIONS(data[i][j][INIT].address, data[i][j][INIT].dram_addr_home_id,
                                       data[i][j][INIT].dstate, data[i][j][INIT].cstate0, data[i][j][INIT].cstate1,
                                       data[i][j][INIT].sharers_list, data[i][j][INIT].d_data, data[i][j][INIT].c_data);
         }
      }
   }

   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);

   if (tid == 1)
   {
      *((UINT32*) dram1_address[0])= write_value;
   }

   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);


   if (tid == 0)
   {
      for (UINT32 i = 0; i < 2; i++)
      {
         for (UINT32 j = 0; j < 2; j++)
         {
            if (ASSERT_MEMORY_STATE(data[i][j][CORE1_WRITE_10].address, data[i][j][CORE1_WRITE_10].dram_addr_home_id,
                                    data[i][j][CORE1_WRITE_10].dstate, data[i][j][CORE1_WRITE_10].cstate0, data[i][j][CORE1_WRITE_10].cstate1,
                                    data[i][j][CORE1_WRITE_10].sharers_list, data[i][j][CORE1_WRITE_10].d_data, data[i][j][CORE1_WRITE_10].c_data, "no error codes yet"))
            {
               ++tests_passed;
            }
            else
            {
               ++tests_failed;
            }
         }
      }
   }

   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);

   if (tid == 0)
   {
      *((UINT32*) dram0_address[0])= write_value;
   }
   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);


   if (tid == 0)
   {
      for (UINT32 i = 0; i < 2; i++)
      {
         for (UINT32 j = 0; j < 2; j++)
         {
            if (ASSERT_MEMORY_STATE(data[i][j][CORE0_WRITE_00].address, data[i][j][CORE0_WRITE_00].dram_addr_home_id,
                                    data[i][j][CORE0_WRITE_00].dstate, data[i][j][CORE0_WRITE_00].cstate0, data[i][j][CORE0_WRITE_00].cstate1,
                                    data[i][j][CORE0_WRITE_00].sharers_list, data[i][j][CORE0_WRITE_00].d_data, data[i][j][CORE0_WRITE_00].c_data, "no error codes yet"))
            {
               ++tests_passed;
            }
            else
            {
               ++tests_failed;
            }
         }
      }
   }

   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);

   if (tid == 0)
   {
      write_value = *((UINT32*) dram0_address[1]);
   }
   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);


   if (tid == 0)
   {
      for (UINT32 i = 0; i < 2; i++)
      {
         for (UINT32 j = 0; j < 2; j++)
         {
            if (ASSERT_MEMORY_STATE(data[i][j][CORE0_READ_01].address, data[i][j][CORE0_READ_01].dram_addr_home_id,
                                    data[i][j][CORE0_READ_01].dstate, data[i][j][CORE0_READ_01].cstate0, data[i][j][CORE0_READ_01].cstate1,
                                    data[i][j][CORE0_READ_01].sharers_list, data[i][j][CORE0_READ_01].d_data, data[i][j][CORE0_READ_01].c_data, "no error codes yet"))
            {
               ++tests_passed;
            }
            else
            {
               ++tests_failed;
            }
         }
      }
   }

   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);

   if (tid == 0)
   {
      *((UINT32*) dram1_address[0]) = write_value;
   }
   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);

   if (tid == 0)
   {
      for (UINT32 i = 0; i < 2; i++)
      {
         for (UINT32 j = 0; j < 2; j++)
         {
            if (ASSERT_MEMORY_STATE(data[i][j][CORE0_WRITE_10].address, data[i][j][CORE0_WRITE_10].dram_addr_home_id,
                                    data[i][j][CORE0_WRITE_10].dstate, data[i][j][CORE0_WRITE_10].cstate0, data[i][j][CORE0_WRITE_10].cstate1,
                                    data[i][j][CORE0_WRITE_10].sharers_list, data[i][j][CORE0_WRITE_10].d_data, data[i][j][CORE0_WRITE_10].c_data, "no error codes yet"))
            {
               ++tests_passed;
            }
            else
            {
               ++tests_failed;
            }
         }
      }
   }

   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);

   if (tid == 0)
   {
      write_value = *((UINT32*) dram1_address[1]);
   }
   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);


   if (tid == 0)
   {
      for (UINT32 i = 0; i < 2; i++)
      {
         for (UINT32 j = 0; j < 2; j++)
         {
            if (ASSERT_MEMORY_STATE(data[i][j][CORE0_READ_11].address, data[i][j][CORE0_READ_11].dram_addr_home_id,
                                    data[i][j][CORE0_READ_11].dstate, data[i][j][CORE0_READ_11].cstate0, data[i][j][CORE0_READ_11].cstate1,
                                    data[i][j][CORE0_READ_11].sharers_list, data[i][j][CORE0_READ_11].d_data, data[i][j][CORE0_READ_11].c_data, "no error codes yet"))
            {
               ++tests_passed;
            }
            else
            {
               ++tests_failed;
            }
         }
      }
   }

   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);

   /********************************************/
   /********************************************/

   if (tid==0)
   {
      cerr << endl << endl;
      cerr << " *** Tile # " << tid << endl;
      cerr << " *** Tests Passed: " << dec << tests_passed << endl;
      cerr << " *** Tests Failed: " << dec << tests_failed << endl;
      cerr << " *** TOTAL TESTS : " << dec << test_count << endl;

      cerr << endl;
      cerr << "********************************************* " << endl
           << " Finished Dual Tile Shared Memory Test Suite  " << endl
           << "********************************************* " << endl;
   }

   //wait for the working core to finish, then we can run the loops above again for the other core
   BARRIER_DUAL_CORE(tid);
}

