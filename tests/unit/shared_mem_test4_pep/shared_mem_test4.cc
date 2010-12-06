/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// File: shared_mem_test4.cc
//
// Description:   This tests the memory model where the PEP and main cores have separate L1 caches, but
//                share an L2 cache.
//
//                A number of threads is spawned on main cores that the main core 0 initially writes to.
//                Each main thread waits on a barrier after each read, so every main thread should be 
//                sequentialy consistent.
//
//                Then a number of helper threads are spawned on PEP cores and the same check is made.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "tile.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

void* thread_func(void*);
void* pep_thread_func(void*);

int num_threads = 2;
int num_iterations = 1;

carbon_barrier_t barrier;

IntPtr address = 0x1000;

int main (int argc, char *argv[])
{
   CarbonStartSim(argc, argv);
   printf("Starting (shared_mem_test4)\n");

   CarbonBarrierInit(&barrier, num_threads);

   carbon_thread_t tid_list[num_threads];

   Tile* tile = Sim()->getTileManager()->getCurrentTile();

   int val = 0;
   tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &val, sizeof(val));
   LOG_PRINT("Tile(%i): Access Time(%llu)\n", tile->getId(), tile->getShmemPerfModel()->getCycleCount());

   for (int i = 0; i < num_threads; i++)
   {
      tid_list[i] = CarbonSpawnThread(thread_func, (void*) i);
   }

   for (int i = 0; i < num_threads; i++)
   {
      CarbonJoinThread(tid_list[i]);
   }

   for (int i = 0; i < num_threads; i++)
   {
      tid_list[i] = CarbonSpawnHelperThread(pep_thread_func, (void*) i);
   }

   for (int i = 0; i < num_threads; i++)
   {
      CarbonJoinHelperThread(tid_list[i]);
   }
  
   int num_misses = (tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val))).first;
   assert(num_misses == 1);
   LOG_PRINT("Tile(%i): Access Time(%llu)\n", tile->getId(), tile->getShmemPerfModel()->getCycleCount());
   
   printf("val = %i\n", val);
   if (val != (2*num_iterations))
   {
      printf("shared_mem_test4 (FAILURE)\n");
   }
   else
   {
      printf("shared_mem_test4 (SUCCESS)\n");
   }
  
   CarbonStopSim();
   return 0;
}

void* thread_func(void* threadid)
{
   long tid = (long) threadid;
   Tile* tile = Sim()->getTileManager()->getCurrentTile();

   for (int i = 0; i < num_iterations; i++)
   {
      int val;
      tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val));
      LOG_PRINT("Tile(%i): Access Time(%llu)\n", tile->getId(), tile->getShmemPerfModel()->getCycleCount());

      printf("read val %d on main core %i\n", val, threadid);

      CarbonBarrierWait(&barrier); 

      if (tid == 0)
      {
         val ++;
         printf("about to write read val %d on pep core %i\n", val, threadid);
         tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &val, sizeof(val));
         LOG_PRINT("Tile(%i): Access Time(%llu)\n", tile->getId(), tile->getShmemPerfModel()->getCycleCount());
      }
   }
   return NULL;
}

void* pep_thread_func(void* threadid)
{
   long tid = (long) threadid;
   Tile* tile = Sim()->getTileManager()->getCurrentTile();

   for (int i = 0; i < num_iterations; i++)
   {
      int val;
      tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val));
      LOG_PRINT("Tile(%i): Access Time(%llu)\n", tile->getId(), tile->getShmemPerfModel()->getCycleCount());

      printf("read val %d on pep core %i\n", val, threadid);
      CarbonBarrierWait(&barrier); 

      if (tid == 0)
      {
         val ++;
      printf("about to write read val %d on pep core %i\n", val, threadid);
         tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &val, sizeof(val));
         LOG_PRINT("Tile(%i): Access Time(%llu)\n", tile->getId(), tile->getShmemPerfModel()->getCycleCount());
   
         int num_misses = (tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val))).first;
      printf("i've got %d in val on pep core %d\n", val, threadid);
         num_misses = (tile->getCore((core_id_t) {1,1})->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val))).first;
      printf("i've got %d in val on pep core 1\n", val);
         num_misses = (tile->getCore((core_id_t) {0,0})->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val))).first;
      printf("i've got %d in val on main core %d\n", val, threadid);
         num_misses = (tile->getCore((core_id_t) {1,0})->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val))).first;
      printf("i've got %d in val on main core 1\n", val);
      }
   }
   return NULL;
}
