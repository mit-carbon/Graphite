#include <cstdlib>
#include <cstring>
#include <vector>
using namespace std;

#include "carbon_user.h"
#include "tile.h"
#include "core.h"
#include "mem_component.h"
#include "memory_manager.h"
#include "cache.h"
#include "simulator.h"
#include "tile_manager.h"
#include "clock_skew_management_object.h"
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
   RD_ONLY_SHARED_MEMORY_READ,
   RD_WR_SHARED_MEMORY_READ,
   RD_WR_SHARED_MEMORY_WRITE,
   PRIVATE_MEMORY_READ,
   PRIVATE_MEMORY_WRITE,
   NUM_INSTRUCTION_TYPES
};

SInt32 m_num_threads;
SInt32 m_degree_of_sharing;
SInt32 m_num_shared_addresses;
SInt32 m_num_private_addresses;
SInt32 m_total_instructions_per_core;
float m_fraction_read_only_shared_addresses;

SInt32 m_log_num_shared_addresses;
SInt32 m_log_cache_block_size;

vector<Time> m_core_clock_list;

vector<Random<float> > m_random_instruction_type_generator;
vector<Random<int> > m_random_rd_only_shared_address_generator;
vector<Random<int> > m_random_rd_wr_shared_address_generator;

float m_instruction_type_probabilities[NUM_INSTRUCTION_TYPES];
vector<UInt64*> m_executed_instruction_types;

// One private address index per thread
vector<SInt32> m_private_address_index;

// A list of shared addresses per thread
vector<vector<IntPtr> > m_rd_only_shared_address_list;
vector<vector<IntPtr> > m_rd_wr_shared_address_list;

carbon_barrier_t m_barrier;

// Function Declarations
void* threadFunc(void*);
void initializeGlobalVariables(int argc, char *argv[]);
void deInitializeGlobalVariables(void);
void computeSharedAddressToThreadMapping(void);
InsType getRandomInstructionType(SInt32 thread_id);
IntPtr getRandomReadOnlySharedAddress(SInt32 thread_id);
IntPtr getRandomReadWriteSharedAddress(SInt32 thread_id);
IntPtr getPrivateAddress(SInt32 thread_id);

int main(int argc, char* argv[])
{
   // Command Line Arguments
   //  -ns <degree of sharing>
   CarbonStartSim(argc, argv);

   // Initialize all the global variables
   initializeGlobalVariables(argc, argv);

   printf("Num Threads(%i)\nDegree of Sharing(%i)\nNum Shared Addresses(%i)\nNum Private Addresses(%i)\nTotal Instructions per Tile(%i)\n\n", m_num_threads, m_degree_of_sharing, m_num_shared_addresses, m_num_private_addresses, m_total_instructions_per_core);

   for (SInt32 i = 0; i < NUM_INSTRUCTION_TYPES; i++)
      printf("Probability[%i] -> %f\n", i, m_instruction_type_probabilities[i]);
   printf("\n");

   // Generate the address to thread mapping
   computeSharedAddressToThreadMapping();

   // Enable all the models
   Simulator::enablePerformanceModelsInCurrentProcess();

   CarbonBarrierInit(&m_barrier, m_num_threads);

   carbon_thread_t thread_list[m_num_threads];
   // Spawn t-1 threads. Directly call the thread func from the main thread
   for (SInt32 i = 1; i < m_num_threads; i++)
   {
      thread_list[i] = CarbonSpawnThread(threadFunc, (void*) i);
   }
   threadFunc((void*) 0);

   // Join t-1 threads
   for (SInt32 i = 1; i < m_num_threads; i++)
   {
      CarbonJoinThread(thread_list[i]);
   }

   // Take the max of all the core clocks
   Time max_clock(0);
   for (SInt32 i = 0; i < m_num_threads; i++)
   {
      if (m_core_clock_list[i] > max_clock)
         max_clock = m_core_clock_list[i];
   }
   
   // Disable all the models
   Simulator::disablePerformanceModelsInCurrentProcess();

   deInitializeGlobalVariables();

   CarbonStopSim();

   printf("Success: Max Tile Clock(%llu ns)\n", (long long unsigned int) max_clock.toNanosec());

   // First, determine the mapping of addresses to threads
   return 0;
}

void* threadFunc(void*)
{
   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   assert((tile->getId() >= 0) && (tile->getId() < m_num_threads));
 
   SInt32 thread_id = tile->getId();

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
            Time ONE_CYCLE = Latency(1, tile->getFrequency());
            m_core_clock_list[thread_id] += ONE_CYCLE;
            break;
         }

      case RD_ONLY_SHARED_MEMORY_READ:
         {
            if (m_rd_only_shared_address_list[thread_id].size() != 0)
            {
               IntPtr address = getRandomReadOnlySharedAddress(thread_id);
               pair<UInt32, Time> ret_val = tile->getCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, 
                     address, (Byte*) &buf, sizeof(buf), true, m_core_clock_list[thread_id]);
               m_core_clock_list[thread_id] += ret_val.second;
            }
            break;
         }

      case RD_WR_SHARED_MEMORY_READ:
         {
            if (m_rd_wr_shared_address_list[thread_id].size() != 0)
            {
               IntPtr address = getRandomReadWriteSharedAddress(thread_id);
               pair<UInt32, Time> ret_val = tile->getCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, 
                     address, (Byte*) &buf, sizeof(buf), true, m_core_clock_list[thread_id]);
               m_core_clock_list[thread_id] += ret_val.second;
            }
            break;
         }

      case RD_WR_SHARED_MEMORY_WRITE:
         {
            if (m_rd_wr_shared_address_list[thread_id].size() != 0)
            {
               IntPtr address = getRandomReadWriteSharedAddress(thread_id);
               pair<UInt32, Time> ret_val = tile->getCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE,
                     address, (Byte*) &buf, sizeof(buf), true, m_core_clock_list[thread_id]);
               m_core_clock_list[thread_id] += ret_val.second;
            }
            break;
         }

      case PRIVATE_MEMORY_READ:
         {
            IntPtr address = getPrivateAddress(thread_id);
            pair<UInt32, Time> ret_val = tile->getCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ,
                  address, (Byte*) &buf, sizeof(buf), true, m_core_clock_list[thread_id]);
            m_core_clock_list[thread_id] += ret_val.second;
            break;
         }

      case PRIVATE_MEMORY_WRITE:
         {
            IntPtr address = getPrivateAddress(thread_id);
            pair<UInt32, Time> ret_val = tile->getCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE,
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
      ClockSkewManagementClient *client = tile->getCore()->getClockSkewManagementClient();
      if (client)
         client->synchronize(m_core_clock_list[thread_id]);
   }

   CarbonBarrierWait(&m_barrier);

   return (void*) NULL;
}

void initializeGlobalVariables(int argc, char *argv[])
{
   // Get data from a file
   cin >> m_num_threads;
   cin >> m_degree_of_sharing;
   cin >> m_num_shared_addresses;
   cin >> m_num_private_addresses;
   cin >> m_total_instructions_per_core;

   // Check what is the current degree of sharing as specified from the command line arguments
   if (strcmp(argv[1], "-ns") == 0)
   {
      // An override has been specified
      // Overwrite the 'degree of sharing'
      m_degree_of_sharing = atoi(argv[2]);
   }

   LOG_ASSERT_ERROR(m_num_threads >= m_degree_of_sharing, "m_num_threads(%i), m_degree_of_sharing(%i)",
         m_num_threads, m_degree_of_sharing);

   LOG_ASSERT_ERROR(isPower2(m_num_shared_addresses) && isPower2(m_num_private_addresses),
         "m_num_shared_addresses(%i), m_num_private_addresses(%i)", m_num_shared_addresses, m_num_private_addresses);

   // This is initially a discrete probablity function
   for (SInt32 i = 0; i < NUM_INSTRUCTION_TYPES; i++)
      cin >> m_instruction_type_probabilities[i];
   
   m_log_num_shared_addresses = floorLog2(m_num_shared_addresses);
   m_log_cache_block_size = floorLog2(Sim()->getCfg()->getInt("l1_dcache/T1/cache_block_size", 0));

   // Do this calculation before converting them into cumulative probabilites
   m_fraction_read_only_shared_addresses = m_instruction_type_probabilities[RD_ONLY_SHARED_MEMORY_READ] / (m_instruction_type_probabilities[RD_ONLY_SHARED_MEMORY_READ] + m_instruction_type_probabilities[RD_WR_SHARED_MEMORY_READ] + m_instruction_type_probabilities[RD_WR_SHARED_MEMORY_WRITE]);

   printf("Fraction Read Only Shared Addresses(%f)\n", m_fraction_read_only_shared_addresses);

   // Convert them into cumulative probabilities
   for (SInt32 i = 1; i < NUM_INSTRUCTION_TYPES; i++)
      m_instruction_type_probabilities[i] = m_instruction_type_probabilities[i] + m_instruction_type_probabilities[i-1];

   m_core_clock_list.resize(m_num_threads);
   m_random_instruction_type_generator.resize(m_num_threads);
   m_random_rd_only_shared_address_generator.resize(m_num_threads);
   m_random_rd_wr_shared_address_generator.resize(m_num_threads);

   m_private_address_index.resize(m_num_threads);
   m_rd_only_shared_address_list.resize(m_num_threads);
   m_rd_wr_shared_address_list.resize(m_num_threads);

   m_executed_instruction_types.resize(m_num_threads);

   for (SInt32 i = 0; i < m_num_threads; i++)
   {
      m_core_clock_list[i] = Time(0);
      m_private_address_index[i] = 0;
      m_random_instruction_type_generator[i].seed(i);
      m_random_rd_only_shared_address_generator[i].seed(i);
      m_random_rd_wr_shared_address_generator[i].seed(i);

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
      IntPtr shared_address = ((IntPtr) i) << m_log_cache_block_size;
      for (SInt32 j = 0; j < m_degree_of_sharing; j++)
      {
         SInt32 thread_id = (SInt32) ((((float) random()) / RAND_MAX) * m_num_threads);
         
         if (i < ((SInt32) (m_fraction_read_only_shared_addresses * m_num_shared_addresses)))
         {
            m_rd_only_shared_address_list[thread_id].push_back(shared_address);
         }
         else
         {
            m_rd_wr_shared_address_list[thread_id].push_back(shared_address);
         }
      }
   }

   for (SInt32 i = 0; i < m_num_threads; i++)
   {
      printf("(%i) rd_only(%u), rd_wr(%u)\n", i, (UInt32) m_rd_only_shared_address_list[i].size(), (UInt32) m_rd_wr_shared_address_list[i].size());
   }

   /*
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
     */
}

InsType getRandomInstructionType(SInt32 thread_id)
{
   float random_num = m_random_instruction_type_generator[thread_id].next(1);

   for (SInt32 i = 0; i < NUM_INSTRUCTION_TYPES; i++)
   {
      if (random_num < m_instruction_type_probabilities[i])
         return (InsType) i;
   }

   // Never reaches here
   assert(false);
   return NUM_INSTRUCTION_TYPES;
}

IntPtr getRandomReadOnlySharedAddress(SInt32 thread_id)
{
   // We get a random shared address from the list
   LOG_ASSERT_ERROR(m_rd_only_shared_address_list[thread_id].size() <= 32768, "Problem With Random Number Generator");

   UInt32 index = m_random_rd_only_shared_address_generator[thread_id].next(m_rd_only_shared_address_list[thread_id].size());
   LOG_PRINT("Thread Id(%i), Index(%u), Address(0x%llx)", thread_id, index, m_rd_only_shared_address_list[thread_id][index]);
   return m_rd_only_shared_address_list[thread_id][index];
}

IntPtr getRandomReadWriteSharedAddress(SInt32 thread_id)
{
   // We get a random shared address from the list
   LOG_ASSERT_ERROR(m_rd_wr_shared_address_list[thread_id].size() <= 32768, "Problem With Random Number Generator");

   UInt32 index = m_random_rd_wr_shared_address_generator[thread_id].next(m_rd_wr_shared_address_list[thread_id].size());
   return m_rd_wr_shared_address_list[thread_id][index];
}

IntPtr getPrivateAddress(SInt32 thread_id)
{
   IntPtr private_address_base = 1 << (m_log_num_shared_addresses + m_log_cache_block_size);
   
   IntPtr curr_private_index = (IntPtr) (thread_id * m_num_private_addresses + m_private_address_index[thread_id]);
   m_private_address_index[thread_id] = (m_private_address_index[thread_id] + 1) % m_num_private_addresses;
   
   return private_address_base + (curr_private_index << m_log_cache_block_size);
}
