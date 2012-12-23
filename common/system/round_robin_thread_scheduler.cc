#include <cassert>
#include <sys/syscall.h>
#include <time.h>
#include "round_robin_thread_scheduler.h"
#include "thread_manager.h"
#include "tile_manager.h"
#include "simulator.h"
#include "network.h"

RoundRobinThreadScheduler::RoundRobinThreadScheduler(ThreadManager* thread_manager, TileManager* tile_manager) 
: ThreadScheduler(thread_manager, tile_manager)
{
}

RoundRobinThreadScheduler::~RoundRobinThreadScheduler()
{
}

void RoundRobinThreadScheduler::requeueThread(core_id_t core_id)
{
   // The round robin scheme simply requeue's the first-in-line to the end.
   ThreadSpawnRequest *req_cpy;
   req_cpy = m_waiter_queue[core_id.tile_id].front();
   m_waiter_queue[core_id.tile_id].pop();
   m_waiter_queue[core_id.tile_id].push(req_cpy);
}

