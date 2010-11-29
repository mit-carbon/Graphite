#include "tile.h"
#include "core.h"
#include "pep_core.h"
#include "network.h"
#include "memory_manager_base.h"
#include "pin_memory_manager.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "clock_skew_minimization_object.h"
#include "core_perf_model.h"
#include "simulator.h"
#include "log.h"



using namespace std;

PepCore::PepCore(Tile* tile) : Core(tile)
{
   //m_core_id = make_core_id(tile->getId(), PEP_CORE_TYPE);
   m_core_id = (core_id_t) {tile->getId(), PEP_CORE_TYPE};
   m_core_perf_model = CorePerfModel::createPepPerfModel((Core *) this); 
   //m_core_perf_model = CorePerfModel::create(tile, Core::PEP_CORE_TYPE);
   
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      assert(tile->getMemoryManager() != NULL);
      assert(tile->getPinMemoryManager() != NULL);
      assert(tile->getShmemPerfModel() != NULL);


      m_memory_manager = tile->getMemoryManager();
      m_pin_memory_manager = tile->getPinMemoryManager();
      m_shmem_perf_model = tile->getShmemPerfModel();
   }
   else
   {
      m_shmem_perf_model = (ShmemPerfModel*) NULL;
      m_memory_manager = (MemoryManagerBase *) NULL;
      m_pin_memory_manager = (PinMemoryManager*) NULL;

      LOG_PRINT("No Memory Manager being used for PEP core");
   }


}

PepCore::~PepCore()
{
   delete m_core_perf_model;
}

