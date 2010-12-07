include "tile.h"
#include "memory_manager_base.h"

#include "shmem_perf_model.h"

#include "tile_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

int main (int argc, char *argv[])
{
   printf("Starting (shmem_perf_model_unit_test)\n");
   CarbonStartSim(argc, argv);

   UInt32 address[2] = {0x0, 0x1000};

   // 1) Get a tile object
   // 2) Get a memory_manager object from it
   // 3) Do initiateSharedMemReq() on the memory_manager object

   Tile* tile = Sim()->getTileManager()->getTileFromIndex(0);
   MemoryManagerBase* memory_manager = tile->getMemoryManager();
   ShmemPerfModel* shmem_perf_model = tile->getShmemPerfModel();

   Byte data_buf[4];
   bool cache_hit;
   UInt64 shmem_time;

   // ACCESS - 0
   shmem_perf_model->setCycleCount(0);
   cache_hit = memory_manager->coreInitiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address[0], 0, data_buf, 4);
   shmem_time = shmem_perf_model->getCycleCount();
   printf("Access(0x%x) - READ : Cache Hit(%s), Shmem Time(%llu)\n", address[0], (cache_hit == true) ? "YES" : "NO", shmem_time);

   // ACCESS - 1
   shmem_perf_model->setCycleCount(0);
   cache_hit = memory_manager->coreInitiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address[1], 0, data_buf, 4);
   shmem_time = shmem_perf_model->getCycleCount();
   printf("Access(0x%x)- WRITE : Cache Hit(%s), Shmem Time(%llu)\n", address[1], (cache_hit == true) ? "YES" : "NO", shmem_time);

   // ACCESS - 2
   shmem_perf_model->setCycleCount(0);
   cache_hit = memory_manager->coreInitiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address[0], 0, data_buf, 4);
   shmem_time = shmem_perf_model->getCycleCount();
   printf("Access(0x%x)- WRITE : Cache Hit(%s), Shmem Time(%llu)\n", address[0], (cache_hit == true) ? "YES" : "NO", shmem_time);

   // ACCESS - 2
   shmem_perf_model->setCycleCount(0);
   cache_hit = memory_manager->coreInitiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address[0], 0, data_buf, 4);
   shmem_time = shmem_perf_model->getCycleCount();
   printf("Access(0x%x)- READ : Cache Hit(%s), Shmem Time(%llu)\n", address[0], (cache_hit == true) ? "YES" : "NO", shmem_time);

   CarbonStopSim();

   printf("Finished (shmem_perf_model_unit_test) - SUCCESS\n");
   return 0;
}
