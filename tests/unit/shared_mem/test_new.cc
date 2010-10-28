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

// FIXME: Modifications
#define STATE_COUNT 6
#define TEST_COUNT 24

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

struct addrVectStruct
{
   IntPtr addr;
   int dram_home_id;
};

struct testStateStruct
{
   DramDirectoryEntry::dstate_t dstate;
   CacheState::cstate_t cstate0;
   CacheState::cstate_t cstate1;
};


/* The final memory state for every test is stored
 * in a number of 'fini_vector' arrays.  It is important
 * to know which tests correspond to which core.
 * therefore, for 52 tests, the first 26 correspond
 * to core#0, and the last 26 correspond to core#1
 */

vector<string> test_code_vector; //store a string of the "Initial Mem State",
vector<string> fini_state_str_vector; //store a string of the "Final Mem State", for debug purposes (" SSI-{0}"
vector<operation_t> operation_vector;
vector<addrVectStruct> address_vector;
vector<testStateStruct> init_test_state_vector;
vector<testStateStruct> fini_test_state_vector;
vector< vector<UINT32> > init_sharers_vector;
vector< vector<UINT32> > fini_sharers_vector;

vector<char*> fini_dram_block_vector;
vector<char*> init_cache_block_vector;
vector<char*> fini_cache_block_vector;

char* dramBlock;
char* cacheBlockS;
char* cacheBlockSW;
char* cacheBlockE;
char* cacheBlockEW;

UINT32 cacheBlockSize;

vector<UINT32> fini_mem_state_id; //allow the "fini state arrays to point to the init state arrays

//dynamically set these values to point
//to valid addresses on dram0, dram1 sections
IntPtr dram0_address, dram1_address;

//vector<stringstream> error_str_vector;

// Function executed by each thread
// each thread "starts" here
void* starter_function(void *threadid);

void awesome_test_suite_msi(int tid);

void initialize_test_parameters();

int main(int argc, char* argv[])   // main begins
{

   if (argc != 2)
   {
      cerr << "[Usage]: ./test <logCacheBlockSize>\n";
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
//   pthread_mutex_lock(&lock);
   cerr << "Waiting to join" << endl << endl;
//   pthread_mutex_unlock(&lock);
#endif

   // Wait for all threads to complete
   pthread_join(threads[0], NULL);
   pthread_join(threads[1], NULL);


#ifdef DEBUG
   cerr << "End of execution" << endl << endl;
#endif

   return 0;
} // main ends

//spawned threads run this function
void* starter_function(void *threadid)
{
   int tid;
   CAPI_Initialize_FreeRank(&tid);

#ifdef DEBUG
   pthread_mutex_lock(&lock);
   cerr << "executing do_nothing function: " << tid << endl << endl;
   pthread_mutex_unlock(&lock);
#endif

   if (tid==0)
   {
      cerr << "Executing awesome test suite Tile # 0 " << endl;
      awesome_test_suite_msi(tid);
      cerr << "FInished Executing awesome test suite  Tile #0" << endl;
   }
   else
   {
      cerr << "Executing awesome test suite Tile #1 " << endl;
      awesome_test_suite_msi(tid);
      cerr << "FInished Executing awesome test suite  Tile #1" << endl;
   }
   // CAPI_Finish(tid);
   pthread_exit(NULL);
}

void BARRIER_DUAL_CORE(int tid)
{
   //this is a stupid barrier just for the test purposes
   int payload;

   cerr << "BARRIER DUAL CORE for ID(" << tid << ")" << endl;
   if (tid==0)
   {
      CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &payload, sizeof(int));
      CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &payload, sizeof(int));
   }
   else
   {
      CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &payload, sizeof(int));
      CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &payload, sizeof(int));
   }
}

void SET_INITIAL_MEM_CONDITIONS(IntPtr address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data, string test_code)
{
   cerr << endl << endl;
   cerr << "   *****************************************************************************************************************" << endl;
   cerr << "   ************* " << test_code << endl;
   cerr << "   ************* " << "ADDR: " << hex << address << " , DramID#: " << dec << dram_address_home_id << endl;
   cerr << "   *****************************************************************************************************************" << endl;

   CAPI_debugSetMemState(address, dram_address_home_id, dstate, cstate0, cstate1, sharers_list, d_data, c_data);
}

bool ASSERT_MEMORY_STATE(IntPtr address, INT32 dram_address_home_id, DramDirectoryEntry::dstate_t dstate, CacheState::cstate_t cstate0, CacheState::cstate_t cstate1, vector<UINT32> sharers_list, char *d_data, char *c_data, string test_code, string error_code)
{
   if (CAPI_debugAssertMemState(address, dram_address_home_id, dstate, cstate0, cstate1, sharers_list, d_data, c_data, test_code, error_code) == 1)
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
   // Responsible for addresses
   IntPtr dram0_address = (IntPtr) &global_array_ptr[0];
   IntPtr dram1_address = (IntPtr) &global_array_ptr[1];

   // dram0_address is aliased to an address homed on core '0'
   // dram1_address is aliased to an address homed on core '1'

   CAPI_alias(dram0_address, DRAM_0, 0);
   CAPI_alias(dram1_address, DRAM_1, 0);  // First cache block in dram0, second cache block also on dram0

   addrVectStruct new_addr_struct;
   new_addr_struct.addr = dram0_address;
   new_addr_struct.dram_home_id = 0;
   address_vector.push_back(new_addr_struct);
   new_addr_struct.addr = dram1_address;
   new_addr_struct.dram_home_id = 1;
   address_vector.push_back(new_addr_struct);

   // Responsible for data
   dramBlock = new char[cacheBlockSize];
   cacheBlockS = new char[cacheBlockSize];
   cacheBlockSW = new char[cacheBlockSize];
   cacheBlockE = new char[cacheBlockSize];
   cacheBlockEW = new char[cacheBlockSize];

   memset(dramBlock, 'a', cacheBlockSize);
   memset(cacheBlockS, 'a', cacheBlockSize);
   memset(cacheBlockSW, 'a', cacheBlockSize);
   memset(cacheBlockE, 'A', cacheBlockSize);
   memset(cacheBlockEW, 'A', cacheBlockSize);

   for (UINT32 i = 0; i < sizeof(UINT32); i++)
   {
      cacheBlockSW[i] = 'z';
      cacheBlockEW[i] = 'z';
   }


   // Rest of code is same
   int state_index = 0;
   int test_index = 0;

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

   testStateStruct new_init_state;
   testStateStruct new_fini_state;

   test_code_vector.resize(STATE_COUNT);
   init_test_state_vector.resize(STATE_COUNT);
   init_sharers_vector.resize(STATE_COUNT);
   init_cache_block_vector.resize(STATE_COUNT);
   /******************************************************/
   //Init Test State#0 III-{}
   test_code_vector[state_index] = ", InitState#0 III-{}";
   new_init_state.dstate = DramDirectoryEntry::UNCACHED;
   new_init_state.cstate0 = CacheState::INVALID;
   new_init_state.cstate1 = CacheState::INVALID;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_null;
   init_cache_block_vector[state_index] = cacheBlockS;
   state_index++;
   /******************************************************/
   //Init Test State#1 SII-{0}
   /*
   test_code_vector[state_index] = ", InitState#1 SII-{0}";
   new_init_state.dstate = DramDirectoryEntry::SHARED;
   new_init_state.cstate0 = CacheState::INVALID;
   new_init_state.cstate1 = CacheState::INVALID;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_0;
   init_cache_block_vector[state_index] = cacheBlockS;
   state_index++;
   */
   /******************************************************/
   //Init Test State#2 SII-{1}
   /*
   test_code_vector[state_index] = ", InitState#2 SII-{1}";
   new_init_state.dstate = DramDirectoryEntry::SHARED;
   new_init_state.cstate0 = CacheState::INVALID;
   new_init_state.cstate1 = CacheState::INVALID;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_1;
   init_cache_block_vector[state_index] = cacheBlockS;
   state_index++;
   */
   /******************************************************/
   //Init Test State#3 SII-{0,1}
   /*
   test_code_vector[state_index] = ", InitState#3 SII-{0,1}";
   new_init_state.dstate = DramDirectoryEntry::SHARED;
   new_init_state.cstate0 = CacheState::INVALID;
   new_init_state.cstate1 = CacheState::INVALID;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_0_1;
   init_cache_block_vector[state_index] = cacheBlockS;
   state_index++;
   */
   /******************************************************/
   //Init Test State#4 EII-{0}
   /*
   test_code_vector[state_index] = ", InitState#4 EII-{0}";
   new_init_state.dstate = DramDirectoryEntry::EXCLUSIVE;
   new_init_state.cstate0 = CacheState::INVALID;
   new_init_state.cstate1 = CacheState::INVALID;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_0;
   init_cache_block_vector[state_index] = cacheBlockE;
   state_index++;
   */
   /******************************************************/
   //Init Test State#5 EII-{1}
   /*
   test_code_vector[state_index] = ", InitState#5 EII-{1}";
   new_init_state.dstate = DramDirectoryEntry::EXCLUSIVE;
   new_init_state.cstate0 = CacheState::INVALID;
   new_init_state.cstate1 = CacheState::INVALID;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_1;
   init_cache_block_vector[state_index] = cacheBlockE;
   state_index++;
   */
   /******************************************************/
   //Init Test State#6 SSI-{0}
   test_code_vector[state_index] = ", InitState#1 SSI-{0}";
   new_init_state.dstate = DramDirectoryEntry::SHARED;
   new_init_state.cstate0 = CacheState::SHARED;
   new_init_state.cstate1 = CacheState::INVALID;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_0;
   init_cache_block_vector[state_index] = cacheBlockS;
   state_index++;
   /******************************************************/
   //Init Test State#7 SSI-{0,1}
   /*
   test_code_vector[state_index] = ", InitState#7 SSI-{0,1}";
   new_init_state.dstate = DramDirectoryEntry::SHARED;
   new_init_state.cstate0 = CacheState::SHARED;
   new_init_state.cstate1 = CacheState::INVALID;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_0_1;
   init_cache_block_vector[state_index] = cacheBlockS;
   state_index++;
   */
   /******************************************************/
   //Init Test State#8 EEI-{0}
   test_code_vector[state_index] = ", InitState#2 EEI-{0}";
   new_init_state.dstate = DramDirectoryEntry::EXCLUSIVE;
   new_init_state.cstate0 = CacheState::EXCLUSIVE;
   new_init_state.cstate1 = CacheState::INVALID;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_0;
   init_cache_block_vector[state_index] = cacheBlockE;
   state_index++;
   /******************************************************/
   //Init Test State#9 SIS-{1}
   test_code_vector[state_index] = ", InitState#3 SIS-{1}";
   new_init_state.dstate = DramDirectoryEntry::SHARED;
   new_init_state.cstate0 = CacheState::INVALID;
   new_init_state.cstate1 = CacheState::SHARED;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_1;
   init_cache_block_vector[state_index] = cacheBlockS;
   state_index++;
   /******************************************************/
   //Init Test State#10 SIS-{0,1}
   /*
   test_code_vector[state_index] = ", InitState#10 SIS-{0,1}";
   new_init_state.dstate = DramDirectoryEntry::SHARED;
   new_init_state.cstate0 = CacheState::INVALID;
   new_init_state.cstate1 = CacheState::SHARED;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_0_1;
   init_cache_block_vector[state_index] = cacheBlockS;
   state_index++;
   */
   /******************************************************/
   //Init Test State#11 SSS-{0,1}
   test_code_vector[state_index] = ", InitState#4 SSS-{0,1}";
   new_init_state.dstate = DramDirectoryEntry::SHARED;
   new_init_state.cstate0 = CacheState::SHARED;
   new_init_state.cstate1 = CacheState::SHARED;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_0_1;
   init_cache_block_vector[state_index] = cacheBlockS;
   state_index++;
   /******************************************************/
   //Init Test State#12 EIE-{1}
   test_code_vector[state_index] = ", InitState#5 EIE-{1}";
   new_init_state.dstate = DramDirectoryEntry::EXCLUSIVE;
   new_init_state.cstate0 = CacheState::INVALID;
   new_init_state.cstate1 = CacheState::EXCLUSIVE;
   init_test_state_vector[state_index] = new_init_state;
   init_sharers_vector[state_index] = sharers_list_1;
   init_cache_block_vector[state_index] = cacheBlockE;
   state_index++;
   /******************************************************/

   /******************************************************/
   /******** SETTING EXPECTED FINAL MEMORY STATES ********/
   /******************************************************/
   cerr << "Setting expected final memory states " << endl;
   operation_vector.resize(TEST_COUNT);
   fini_mem_state_id.resize(TEST_COUNT);
   fini_test_state_vector.resize(TEST_COUNT);
   fini_sharers_vector.resize(TEST_COUNT);
   fini_state_str_vector.resize(TEST_COUNT);
   fini_dram_block_vector.resize(TEST_COUNT);
   fini_cache_block_vector.resize(TEST_COUNT);

   /******************************************************/
   /*****************     CORE#0 READ   ******************/
   /******************************************************/

   /******************************************************/
   //III-{} -> SSI-{0}
   cerr << "Test_Index should be 0: " << test_index << endl;
   fini_mem_state_id[test_index] = 0;
   fini_state_str_vector[test_index] = "SSI-{0}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::INVALID;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   /******************************************************/
   //SII-{0} -> SSI-{0}
   /*
   fini_mem_state_id[test_index] = 1;
   fini_state_str_vector[test_index] = "SSI-{0}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::INVALID;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //SII-{1} -> SSI-{0,1}
   /*
   fini_mem_state_id[test_index] = 2;
   fini_state_str_vector[test_index] = "SSI-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::INVALID;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //SII-{0,1} -> SSI-{0,1}
   /*
   fini_mem_state_id[test_index] = 3;
   fini_state_str_vector[test_index] = "SSI-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::INVALID;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //EII-{0} -> SSI-{0}
   /*
   fini_mem_state_id[test_index] = 4;
   fini_state_str_vector[test_index] = "SSI-{0}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::INVALID;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */

   /******************************************************/
   //EII-{1} -> SSI-{0}

   /*
   fini_mem_state_id[test_index] = 5;
   fini_state_str_vector[test_index] = "SSI-{0}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::INVALID;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */

   /******************************************************/
   //SSI-{0} -> SSI-{0}
   fini_mem_state_id[test_index] = 1;
   fini_state_str_vector[test_index] = "SSI-{0}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::INVALID;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   /******************************************************/
   //SSI-{0,1} -> SSI-{0,1}
   /*
   fini_mem_state_id[test_index] = 7;
   fini_state_str_vector[test_index] = "SSI-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::INVALID;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //EEI-{0} -> EEI-{0}
   fini_mem_state_id[test_index] = 2;
   fini_state_str_vector[test_index] = "EEI-{0}";
   new_fini_state.dstate = DramDirectoryEntry::EXCLUSIVE;
   new_fini_state.cstate0 = CacheState::EXCLUSIVE;
   new_fini_state.cstate1 = CacheState::INVALID;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockE;
   ++test_index;
   /******************************************************/
   //SIS-{1} -> SSS-{0,1}
   fini_mem_state_id[test_index] = 3;
   fini_state_str_vector[test_index] = "SSS-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   /******************************************************/
   //SIS-{0,1} -> SSS-{0,1}
   /*
   fini_mem_state_id[test_index] = 10;
   fini_state_str_vector[test_index] = "SSS-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //SSS-{0,1} -> SSS-{0,1}
   fini_mem_state_id[test_index] = 4;
   fini_state_str_vector[test_index] = "SSS-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   /******************************************************/
   //EIE-{1} -> SSS-{0,1}
   fini_mem_state_id[test_index] = 5;
   fini_state_str_vector[test_index] = "SSS-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_0_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockE;
   fini_cache_block_vector[test_index] = cacheBlockE;
   ++test_index;
   /******************************************************/
   /*****************    CORE#0 WRITE   ******************/
   /******************************************************/

   /******************************************************/
   //III-{} -> EEI-{0}
   fini_mem_state_id[test_index] = 0;
   fini_state_str_vector[test_index] = "EEI-{0}";
   new_fini_state.dstate = DramDirectoryEntry::EXCLUSIVE;
   new_fini_state.cstate0 = CacheState::EXCLUSIVE;
   new_fini_state.cstate1 = CacheState::INVALID;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   /******************************************************/
   //SII-{0} -> EEI-{0}
   /*
   fini_mem_state_id[test_index] = 1;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //SII-{1} -> EEI-{0}
   /*
   fini_mem_state_id[test_index] = 2;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //SII-{0,1} -> EEI-{0}
   /*
   fini_mem_state_id[test_index] = 3;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //EII-{0} -> EEI-{0}
   /*
   fini_mem_state_id[test_index] = 4;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //EII-{1} -> EEI-{0}
   /*
   fini_mem_state_id[test_index] = 5;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //SSI-{0} -> EEI-{0}
   fini_mem_state_id[test_index] = 1;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   /******************************************************/
   //SSI-{0,1} -> EEI-{0}
   /*
   fini_mem_state_id[test_index] = 7;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //EEI-{0} -> EEI-{0}
   fini_mem_state_id[test_index] = 2;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockEW;
   ++test_index;
   /******************************************************/
   //SIS-{1} -> EEI-{0}
   fini_mem_state_id[test_index] = 3;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   /******************************************************/
   //SIS-{0,1} -> EEI-{0}
   /*
   fini_mem_state_id[test_index] = 10;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //SSS-{0,1} -> EEI-{0}
   fini_mem_state_id[test_index] = 4;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   /******************************************************/
   //EIE-{1} -> EEI-{0}
   fini_mem_state_id[test_index] = 5;
   fini_state_str_vector[test_index] = "EEI-{0}";
   //EEI-{0} is consistent throughout the entire CORE#0 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0;
   operation_vector[test_index] = CORE_0_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockE;
   fini_cache_block_vector[test_index] = cacheBlockEW;
   ++test_index;
   /******************************************************/
   /******************************************************/

   /******************************************************/
   /*****************     CORE#1 READ   ******************/
   /******************************************************/

   /******************************************************/
   //III-{} -> SIS-{1}
   fini_mem_state_id[test_index] = 0;
   fini_state_str_vector[test_index] = "SIS-{1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::INVALID;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   /******************************************************/
   //SII-{0} -> SIS-{1}
   /*
   fini_mem_state_id[test_index] = 1;
   fini_state_str_vector[test_index] = "SIS-{1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::INVALID;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //SII-{1} -> SIS-{1}
   /*
   fini_mem_state_id[test_index] = 2;
   fini_state_str_vector[test_index] = "SIS-{1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::INVALID;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //SII-{0,1} -> SIS-{0,1}
   /*
   fini_mem_state_id[test_index] = 3;
   fini_state_str_vector[test_index] = "SIS-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::INVALID;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //EII-{0} -> SIS-{1}
   /*
   fini_mem_state_id[test_index] = 4;
   fini_state_str_vector[test_index] = "SIS-{1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::INVALID;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //EII-{1} -> SIS-{1}
   /*
   fini_mem_state_id[test_index] = 5;
   fini_state_str_vector[test_index] = "SIS-{1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::INVALID;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //SSI-{0} -> SSS-{0,1}
   fini_mem_state_id[test_index] = 1;
   fini_state_str_vector[test_index] = "SSS-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   /******************************************************/
   //SSI-{0,1} -> SSS-{0,1}
   /*
   fini_mem_state_id[test_index] = 7;
   fini_state_str_vector[test_index] = "SSS-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //EEI-{0} -> SSS-{0,1}
   fini_mem_state_id[test_index] = 2;
   fini_state_str_vector[test_index] = "SSS-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockE;
   fini_cache_block_vector[test_index] = cacheBlockE;
   ++test_index;
   /******************************************************/
   //SIS-{1} -> SIS-{1}
   fini_mem_state_id[test_index] = 3;
   fini_state_str_vector[test_index] = "SIS-{1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::INVALID;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   /******************************************************/
   //SIS-{0,1} -> SIS-{0,1}
   /*
   fini_mem_state_id[test_index] = 10;
   fini_state_str_vector[test_index] = "SIS-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::INVALID;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   */
   /******************************************************/
   //SSS-{0,1} -> SSS-{0,1}
   fini_mem_state_id[test_index] = 4;
   fini_state_str_vector[test_index] = "SSS-{0,1}";
   new_fini_state.dstate = DramDirectoryEntry::SHARED;
   new_fini_state.cstate0 = CacheState::SHARED;
   new_fini_state.cstate1 = CacheState::SHARED;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_0_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockS;
   ++test_index;
   /******************************************************/
   //EIE-{1} -> EIE-{1}
   fini_mem_state_id[test_index] = 5;
   fini_state_str_vector[test_index] = "EIE-{1}";
   new_fini_state.dstate = DramDirectoryEntry::EXCLUSIVE;
   new_fini_state.cstate0 = CacheState::INVALID;
   new_fini_state.cstate1 = CacheState::EXCLUSIVE;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_READ_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockE;
   ++test_index;
   /******************************************************/
   /*****************    CORE#1 WRITE   ******************/
   /******************************************************/

   /******************************************************/
   //III-{} -> EEI-{1}
   fini_mem_state_id[test_index] = 0;
   fini_state_str_vector[test_index] = "EEI-{1}";
   new_fini_state.dstate = DramDirectoryEntry::EXCLUSIVE;
   new_fini_state.cstate0 = CacheState::INVALID;
   new_fini_state.cstate1 = CacheState::EXCLUSIVE;
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   /******************************************************/
   //SII-{0} -> EEI-{1}
   /*
   fini_mem_state_id[test_index] = 1;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //SII-{1} -> EEI-{1}
   /*
   fini_mem_state_id[test_index] = 2;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //SII-{0,1} -> EEI-{1}
   /*
   fini_mem_state_id[test_index] = 3;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //EII-{0} -> EEI-{1}
   /*
   fini_mem_state_id[test_index] = 4;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //EII-{1} -> EEI-{1}
   /*
   fini_mem_state_id[test_index] = 5;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //SSI-{0} -> EEI-{1}
   fini_mem_state_id[test_index] = 1;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   /******************************************************/
   //SSI-{0,1} -> EEI-{1}
   /*
   fini_mem_state_id[test_index] = 7;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //EEI-{0} -> EEI-{1}
   fini_mem_state_id[test_index] = 2;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockE;
   fini_cache_block_vector[test_index] = cacheBlockEW;
   ++test_index;
   /******************************************************/
   //SIS-{1} -> EEI-{1}
   fini_mem_state_id[test_index] = 3;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   /******************************************************/
   //SIS-{0,1} -> EEI-{1}
   /*
   fini_mem_state_id[test_index] = 10;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   */
   /******************************************************/
   //SSS-{0,1} -> EEI-{1}
   fini_mem_state_id[test_index] = 4;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockSW;
   ++test_index;
   /******************************************************/
   //EIE-{1} -> EEI-{1}
   fini_mem_state_id[test_index] = 5;
   fini_state_str_vector[test_index] = "EEI-{1}";
   //EEI-{1} is consistent throughout the entire CORE#1 WRITE tests
   fini_test_state_vector[test_index] = new_fini_state;
   fini_sharers_vector[test_index] = sharers_list_1;
   operation_vector[test_index] = CORE_1_WRITE_OP;
   fini_dram_block_vector[test_index] = cacheBlockS;
   fini_cache_block_vector[test_index] = cacheBlockEW;
   ++test_index;
   /******************************************************/

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
   stringstream test_code;//, error_str;

   int dram_addr_home_id = 0;

   DramDirectoryEntry::dstate_t init_dstate;
   DramDirectoryEntry::dstate_t fini_dstate;
   CacheState::cstate_t init_cstate0;
   CacheState::cstate_t init_cstate1;
   CacheState::cstate_t fini_cstate0;
   CacheState::cstate_t fini_cstate1;

   char *init_dram_block;
   char *fini_dram_block;
   char *init_cache_block;
   char *fini_cache_block;

   int state_index = 0;
   IntPtr address;

   vector<UINT32> init_sharers_list;
   vector<UINT32> fini_sharers_list;

   /********************************************/
   /********************************************/
   /********************************************/

   for (int core_id=0; core_id < CORE_COUNT; core_id++)
   {

      if (tid == core_id)
      {
         //execute all of the tests below

         //loop through all of the addresses (homed on diffrent DRAMs...)
         for (int addr_index = 0; addr_index < (int) address_vector.size(); addr_index++)
         {
            address = address_vector[addr_index].addr;
            dram_addr_home_id = address_vector[addr_index].dram_home_id;

//    cerr << endl << endl;
//    cerr << "****************************************" << endl;
//    cerr << "****************************************" << endl;
//    cerr << "************  NEW ADDRESS  *************" << endl;
//    cerr << "****************************************" << endl;
//    cerr << "****************************************" << endl << endl;

            //loop through all of the test cases
            for (int test_index = core_id * (fini_test_state_vector.size() / 2); test_index < (int)((core_id + 1) * (fini_test_state_vector.size() / 2)); test_index++)
            {
               //starts counting from Test#1
               ++test_count;
               //the fini arrays have to specify what the initial parameters are (which must be pulled from the init arrays)
               state_index =  fini_mem_state_id[test_index];

               operation_t operation = operation_vector[test_index];

               test_code.str(""); //error_str.str("");
               test_code << "Test #" << test_count << "Tindex[" << test_index << "], State#:" << state_index << " " << " -- Tile # " << tid << " OP: "
               << operationToString(operation) << " ," <<  test_code_vector[state_index] << " -> " << fini_state_str_vector[test_index];
               //    error_str = error_str_vector[test_index];

               init_dstate = init_test_state_vector[state_index].dstate;
               init_cstate0 = init_test_state_vector[state_index].cstate0;
               init_cstate1 = init_test_state_vector[state_index].cstate1;
               init_sharers_list = init_sharers_vector[state_index];

               init_dram_block = dramBlock;
               init_cache_block = init_cache_block_vector[state_index];

               fini_dstate = fini_test_state_vector[test_index].dstate;
               fini_cstate0 = fini_test_state_vector[test_index].cstate0;
               fini_cstate1 = fini_test_state_vector[test_index].cstate1;
               fini_sharers_list = fini_sharers_vector[test_index];

               fini_dram_block = fini_dram_block_vector[test_index];
               fini_cache_block = fini_cache_block_vector[test_index];

               /* I need a mechanism here to do "addr_home_lookup()" */
               SET_INITIAL_MEM_CONDITIONS(address, dram_addr_home_id, init_dstate, init_cstate0, init_cstate1, init_sharers_list, (char*) init_dram_block, (char*) init_cache_block, test_code.str());

               switch (operation)
               {
               case CORE_0_READ_OP:
                  //write_value misnomer
                  //READ OPERATION
                  if (tid==0)
                     write_value = *((UINT32*) address);
                  break;
               case CORE_0_WRITE_OP:
                  //WRITE OPERATION
                  if (tid==0)
                     *((UINT32*) address) = write_value;
                  break;
               case CORE_1_READ_OP:
                  //write_value misnomer
                  //READ OPERATION
                  if (tid==1)
                     write_value = *((UINT32*) address);
                  break;
               case CORE_1_WRITE_OP:
                  //WRITE OPERATION
                  if (tid==1)
                     *((UINT32*) address) = write_value;
                  break;
               default:
                  cerr << "ERROR IN OPERATION SWITCH " << endl;
                  break;
               }

               if (ASSERT_MEMORY_STATE(address, dram_addr_home_id, fini_dstate, fini_cstate0, fini_cstate1, fini_sharers_list, fini_dram_block, fini_cache_block, test_code.str(), "no error codes yet")) //error_str.str());
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
      else
      {
         //other core who does no work
      }

      //wait for the working core to finish, then we can run the loops above again for the other core
      BARRIER_DUAL_CORE(tid);
   }

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

}

