#pragma once

#include <vector>

#include "fixed_types.h"
#include "packetize.h"

// Forward Decls
class ThreadManager;
class Network;

class LaxBarrierSyncServer : public ClockSkewManagementServer
{
private:
   Network &m_network;
   UnstructuredBuffer &m_recv_buff;
   ThreadManager* m_thread_manager;

   UInt64 m_barrier_interval;
   UInt64 m_next_barrier_time;
   std::vector<UInt64> m_local_clock_list;
   std::vector<bool> m_barrier_acquire_list;
   
   UInt32 m_num_application_tiles;

public:
   LaxBarrierSyncServer(Network &network, UnstructuredBuffer &recv_buff);
   ~LaxBarrierSyncServer();

   void processSyncMsg(core_id_t core_id);
   void signal();

   void barrierWait(core_id_t core_id);
   bool isBarrierReached(void);
   void barrierRelease(void);
};
