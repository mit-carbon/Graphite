#include <stdlib.h>
#include <vector>
using namespace std;

#include "carbon_user.h"
#include "core.h"
#include "simulator.h"
#include "core_manager.h"
#include "clock_skew_minimization_object.h"
#include "config.h"
#include "random.h"
#include "fixed_types.h"
#include "utils.h"

// 1/3rd instructions are memory accesses

// 2/3rd of all memory accesses are loads
// 1/3rd of all memory accesses are stores

// 1/2 of memory accesses are to private data
// 1/2 of memory accesses are to shared data

enum InsType
{
   NON_MEMORY = 0,
   SHARED_MEMORY_READ,
   SHARED_MEMORY_WRITE,
   PRIVATE_MEMORY_READ,
   PRIVATE_MEMORY_WRITE,
   NUM_INSTRUCTION_TYPES
};

SInt32 m_num_threads;
SInt32 m_degree_of_sharing;
SInt32 m_num_shared_addresses;
SInt32 m_num_private_addresses;
SInt32 m_total_instructions_per_core;

SInt32 m_log_num_shared_addresses;
SInt32 m_log_cache_block_size;

vector<UInt64> m_core_clock_list;

vector<Random> m_random_instruction_type_generator;
vector<Random> m_random_address_generator;

float m_instruction_type_probabilities[NUM_INSTRUCTION_TYPES];
vector<UInt64*> m_executed_instruction_types;

// One private address index per thread
vector<SInt32> m_private_address_index;

// A list of shared addresses per thread
vector<vector<IntPtr> > m_shared_address_list;

carbon_barrier_t m_barrier;

// Function Declarations
void* thread_func(void*);
void initializeGlobalVariables(void);
void deInitializeGlobalVariables(void);
void computeSharedAddressToThreadMapping(void);
InsType getRandomInstructionType(SInt32 thread_id);
IntPtr getRandomSharedAddress(SInt32 thread_id);
IntPtr getPrivateAddress(SInt32 thread_id);

int main(int argc, char* argv[])
{
   // Command Line Arguments
   //  -n <num_shared_addresses>
   //  -t <num_threads>
   //  -d <degree of sharing>
   CarbonStartSim(argc, argv);

   // Initialize all the global variables
   initializeGlobalVariables();

   printf("Num Threads(%i)\nDegree of Sharing(%i)\nNum Shared Addresses(%i)\nNum Private Addresses(%i)\nTotal Instructions per Core(%i)\n\n", m_num_threads, m_degree_of_sharing, m_num_shared_addresses, m_num_private_addresses, m_total_instructions_per_core);

   for (SInt32 i = 0; i < NUM_INSTRUCTION_TYPES; i++)
      printf("Probability[%i] -> %f\n", i, m_instruction_type_probabilities[i]);
   printf("\n");

   // Generate the address to thread mapping
   computeSharedAddressToThreadMapping();

   // Enable all the models
   for (SInt32 i = 0; i < (SInt32) Config::getSingleton()->getTotalCores(); i++)
      Sim()->getCoreManager()->getCoreFromID(i)->enablePerformanceModels();

   CarbonBarrierInit(&m_barrier, m_num_threads);

   carbon_thread_t thread_list[m_num_threads];
   // Spawn t-1 threads. Directly call the thread func from the main thread
   for (SInt32 i = 1; i < m_num_threads; i++)
   {
      thread_list[i] = CarbonSpawnThread(thread_func, (void*) i);
   }
   thread_func((void*) 0);

   // Join t-1 threads
   for (SInt32 i = 1; i < m_num_threads; i++)
   {
      CarbonJoinThread(thread_list[i]);
   }

   // Take the max of all the core clocks
   UInt64 max_clock = 0;
   for (SInt32 i = 0; i < m_num_threads; i++)
   {
      if (m_core_clock_list[i] > max_clock)
         max_clock = m_core_clock_list[i];
   }
   
   // Disable all the models
   for (SInt32 i = 0; i < (SInt32) Config::getSingleton()->getTotalCores(); i++)
      Sim()->getCoreManager()->getCoreFromID(i)->disablePerformanceModels();

   deInitializeGlobalVariables();

   CarbonStopSim();

   printf("Success: Max Core Clock(%llu)\n", (long long unsigned int) max_clock);

   // First, determine the mapping of addresses to threads
   return 0;
}

void* thread_func(void*)
{
   Core* core = Sim()->getCoreManager()->getCurrentCore();
   assert((core->getId() >= 0) && (core->getId() < m_num_threads));
 
   SInt32 thread_id = core->getId();

   SInt32 num_instructions_simulated = 0; 
   while(num_instructions_simulated < m_total_instructions_per_core)
   {
      UInt32 buf;

      // Get random number
      // What it will tell you
      // 1) Normal Instruction / Load Store
      InsType ins_type = getRandomInstructionType(thread_id);

      m_executed_instruction_types[thread_id][ins_type] ++;

      switch(ins_type)
      {
         case NON_MEMORY:
            {
               m_core_clock_list[thread_id] += 1;
               break;
            }

         case SHARED_MEMORY_READ:
            {
               IntPtr address = getRandomSharedAddress(thread_id);
               pair<UInt32, UInt64> ret_val = core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, 
                     address, (Byte*) &buf, sizeof(buf), true, m_core_clock_list[thread_id]);
               m_core_clock_list[thread_id] += ret_val.second;
               break;
            }

         case SHARED_MEMORY_WRITE:
            {
               IntPtr address = getRandomSharedAddress(thread_id);
               pair<UInt32, UInt64> ret_val = core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE,
                     address, (Byte*) &buf, sizeof(buf), true, m_core_clock_list[thread_id]);
               m_core_clock_list[thread_id] += ret_val.second;
               break;
            }

         case PRIVATE_MEMORY_READ:
            {
               IntPtr address = getPrivateAddress(thread_id);
               pair<UInt32, UInt64> ret_val = core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ,
                     address, (Byte*) &buf, sizeof(buf), true, m_core_clock_list[thread_id]);
               m_core_clock_list[thread_id] += ret_val.second;
               break;
            }

         case PRIVATE_MEMORY_WRITE:
            {
               IntPtr address = getPrivateAddress(thread_id);
               pair<UInt32, UInt64> ret_val = core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE,
                     address, (Byte*) &buf, sizeof(buf), true, m_core_clock_list[thread_id]);
               m_core_clock_list[thread_id] += ret_val.second;
               break;
            }

         default:
            LOG_PRINT_ERROR("Unrecognized Instruction Type(%u)", ins_type);
            break;
      }

      num_instructions_simulated ++;
      
      // Synchronize the clocks
      ClockSkewMinimizationClient *client = core->getClockSkewMinimizationClient();
      assert(client);

      client->synchronize(m_core_clock_list[thread_id]);
   }

   CarbonBarrierWait(&m_barrier);

   return (void*) NULL;
}

void initializeGlobalVariables()
{
   // Get data from a file
   cin >> m_num_threads;
   cin >> m_degree_of_sharing;
   cin >> m_num_shared_addresses;
   cin >> m_num_private_addresses;
   cin >> m_total_instructions_per_core;

   LOG_ASSERT_ERROR(m_num_threads >= m_degree_of_sharing, "m_num_threads(%i), m_degree_of_sharing(%i)",
         m_num_threads, m_degree_of_sharing);

   LOG_ASSERT_ERROR(isPower2(m_num_shared_addresses) && isPower2(m_num_private_addresses),
         "m_num_shared_addresses(%i), m_num_private_addresses(%i)", m_num_shared_addresses, m_num_private_addresses);

   // This is initially a discrete probablity function
   for (SInt32 i = 0; i < NUM_INSTRUCTION_TYPES; i++)
      cin >> m_instruction_type_probabilities[i];
   
   m_log_num_shared_addresses = floorLog2(m_num_shared_addresses);
   m_log_cache_block_size = floorLog2(Sim()->getCfg()->getInt("perf_model/l1_dcache/cache_block_size", 0));

   // Convert them into cumulative probabilities
   for (SInt32 i = 1; i < NUM_INSTRUCTION_TYPES; i++)
      m_instruction_type_probabilities[i] = m_instruction_type_probabilities[i] + m_instruction_type_probabilities[i-1];

   m_core_clock_list.resize(m_num_threads);
   m_random_instruction_type_generator.resize(m_num_threads);
   m_random_address_generator.resize(m_num_threads);

   m_private_address_index.resize(m_num_threads);
   m_shared_address_list.resize(m_num_threads);

   m_executed_instruction_types.resize(m_num_threads);

   for (SInt32 i = 0; i < m_num_threads; i++)
   {
      m_core_clock_list[i] = 0;
      m_private_address_index[i] = 0;
      m_random_instruction_type_generator[i].seed(i);
      m_random_address_generator[i].seed(i);

      m_executed_instruction_types[i] = new UInt64[NUM_INSTRUCTION_TYPES];
      for (SInt32 j = 0; j < NUM_INSTRUCTION_TYPES; j++)
         m_executed_instruction_types[i][j] = 0;
   }
}

void deInitializeGlobalVariables()
{
   for (SInt32 i = 0; i < m_num_threads; i++)
   {
      printf("thread(%i) -> ", i);
      for (SInt32 j = 0; j < NUM_INSTRUCTION_TYPES; j++)
      {
         printf("%llu\t", (long long unsigned int) m_executed_instruction_types[i][j]);
      }
      printf("\n");

      delete [] m_executed_instruction_types[i];
   }
}

void computeSharedAddressToThreadMapping()
{
   srandom(1);
   // This is pre-computed before the application starts
   for (SInt32 i = 0; i < m_num_shared_addresses; i++)
   {
      IntPtr shared_address = i << m_log_cache_block_size;
      for (SInt32 j = 0; j < m_degree_of_sharing; j++)
      {
         SInt32 thread_id = (SInt32) ((((float) random()) / RAND_MAX) * m_num_threads);
         m_shared_address_list[thread_id].push_back(shared_address);
      }
   }

   UInt32 max_shared_address_list_size = 0;
   UInt32 min_shared_address_list_size = 1000000;

   for (SInt32 i = 0; i < m_num_threads; i++)
   {
      if (m_shared_address_list[i].size() > max_shared_address_list_size)
         max_shared_address_list_size = m_shared_address_list[i].size();
      if (m_shared_address_list[i].size() < min_shared_address_list_size)
         min_shared_address_list_size = m_shared_address_list[i].size();
   }
   printf("max_shared_address_list_size(%u), min_shared_address_list_size(%u)\n", \
         max_shared_address_list_size, min_shared_address_list_size);
}

InsType getRandomInstructionType(SInt32 thread_id)
{
   float random_num = ((float) m_random_instruction_type_generator[thread_id].next(32768)) / 32768;

   for (SInt32 i = 0; i < NUM_INSTRUCTION_TYPES; i++)
   {
      if (random_num < m_instruction_type_probabilities[i])
         return (InsType) i;
   }

   // Never reaches here
   assert(false);
   return NUM_INSTRUCTION_TYPES;
}

IntPtr getRandomSharedAddress(SInt32 thread_id)
{
   // We get a random shared address from the list
   LOG_ASSERT_ERROR(m_shared_address_list[thread_id].size() <= 32768, "Problem With Random Number Generator");

   UInt32 index = m_random_address_generator[thread_id].next(m_shared_address_list[thread_id].size());
   return m_shared_address_list[thread_id][index];
}

IntPtr getPrivateAddress(SInt32 thread_id)
{
   IntPtr private_address_base = 1 << (m_log_num_shared_addresses + m_log_cache_block_size);
   
   IntPtr curr_private_index = (IntPtr) (thread_id * m_num_private_addresses + m_private_address_index[thread_id]);
   m_private_address_index[thread_id] = (m_private_address_index[thread_id] + 1) % m_num_private_addresses;
   
   return private_address_base + (curr_private_index << m_log_cache_block_size);
}
