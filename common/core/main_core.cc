#include "tile.h"
#include "core.h"
#include "main_core.h"
#include "network.h"
#include "memory_manager_base.h"
#include "pin_memory_manager.h"
#include "core_perf_model.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "clock_skew_minimization_object.h"
#include "simulator.h"
#include "log.h"

using namespace std;

MainCore::MainCore(Tile* tile) : Core(tile)
{
   //m_core_id = make_core_id(tile->getId(), MAIN_CORE_TYPE);
   m_core_id = (core_id_t) {tile->getId(), MAIN_CORE_TYPE};
   m_core_perf_model = CorePerfModel::createMainPerfModel((Core *) this);
   //m_core_perf_model = CorePerfModel::create(tile, CORE::MAIN_CORE_TYPE);
  
   //m_network = new Network(tile);

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      m_shmem_perf_model = new ShmemPerfModel();
      LOG_PRINT("instantiated shared memory performance model");

      m_memory_manager = MemoryManagerBase::createMMU(
            Sim()->getCfg()->getString("caching_protocol/type"),
            m_tile, m_tile->getNetwork(), m_shmem_perf_model);
      LOG_PRINT("instantiated memory manager model");

      m_pin_memory_manager = new PinMemoryManager(tile);

      tile->setMemoryManager(m_memory_manager);
      tile->setPinMemoryManager(m_pin_memory_manager);
      tile->setShmemPerfModel(m_shmem_perf_model);
   }
   else
   {
      m_shmem_perf_model = (ShmemPerfModel*) NULL;
      m_memory_manager = (MemoryManagerBase *) NULL;
      m_pin_memory_manager = (PinMemoryManager*) NULL;

      LOG_PRINT("No Memory Manager being used for main core");
   }


   m_clock_skew_minimization_client = ClockSkewMinimizationClient::create(Sim()->getCfg()->getString("clock_skew_minimization/scheme","none"), this);
}

MainCore::~MainCore()
{
   delete m_core_perf_model;
   if (m_clock_skew_minimization_client)
      delete m_clock_skew_minimization_client;
   
   delete m_pin_memory_manager;
   delete m_memory_manager;
   delete m_shmem_perf_model;
}

