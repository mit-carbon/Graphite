#ifndef __BARRIER_SYNC_SERVER_H__
#define __BARRIER_SYNC_SERVER_H__

#include <vector>

#include "fixed_types.h"
#include "packetize.h"

// Forward Decls
class ThreadManager;
class Network;

class BarrierSyncServer : public ClockSkewMinimizationServer
{
   private:
      Network &m_network;
      UnstructuredBuffer &m_recv_buff;
      ThreadManager* m_thread_manager;

      UInt64 m_barrier_interval;
      UInt64 m_next_barrier_time;
      std::vector<UInt64> m_local_clock_list;
      std::vector<bool> m_barrier_acquire_list;

      std::vector<UInt64> m_local_pep_clock_list;
      std::vector<bool> m_barrier_pep_acquire_list;
      
      UInt32 m_num_application_tiles;

   public:
      BarrierSyncServer(Network &network, UnstructuredBuffer &recv_buff);
      ~BarrierSyncServer();

      void processSyncMsg(core_id_t core_id);
      void signal();

      void barrierWait(core_id_t core_id);
      bool isBarrierReached(void);
      void barrierRelease(void);
};

#endif /* __BARRIER_SYNC_SERVER_H__ */
