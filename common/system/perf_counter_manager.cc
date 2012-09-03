#include "tile.h"
#include "tile_manager.h"
#include "perf_counter_manager.h"
#include "log.h"
#include "packet_type.h"
#include "simulator.h"
#include "network.h"

PerfCounterManager::PerfCounterManager(ThreadManager *thread_manager):
   m_thread_manager(thread_manager)
{}

PerfCounterManager::~PerfCounterManager()
{}
