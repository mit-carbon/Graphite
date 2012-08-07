#ifndef ROUND_ROBIN_THREAD_SCHEDULER_H
#define ROUND_ROBIN_THREAD_SCHEDULER_H

#include "thread_scheduler.h"

class ThreadManager;
class TileManager;

class RoundRobinThreadScheduler : public ThreadScheduler
{
public:
   RoundRobinThreadScheduler(ThreadManager *thread_manager, TileManager *tile_manager);
   ~RoundRobinThreadScheduler();


   virtual void requeueThread(core_id_t core_id);
};

#endif // ROUND_ROBIN_THREAD_SCHEDULER_H
