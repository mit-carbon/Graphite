#include "lax_barrier_sync_client.h"
#include "lax_barrier_sync_server.h"
#include "simulator.h"
#include "thread_manager.h"
#include "tile_manager.h"
#include "network.h"
#include "tile.h"
#include "config.h"
#include "statistics_thread.h"
#include "log.h"

LaxBarrierSyncServer::LaxBarrierSyncServer(Network &network, UnstructuredBuffer &recv_buff):
   m_network(network),
   m_recv_buff(recv_buff)
{
   m_thread_manager = Sim()->getThreadManager();
   try
   {
      m_barrier_interval = (UInt64) Sim()->getCfg()->getInt("clock_skew_management/lax_barrier/quantum"); 
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error Reading 'clock_skew_management/barrier/quantum' from the config file");
   }

   m_next_barrier_time = m_barrier_interval;
   m_num_application_tiles = Config::getSingleton()->getApplicationTiles();
   m_local_clock_list.resize(m_num_application_tiles);
   m_barrier_acquire_list.resize(m_num_application_tiles);

   for (UInt32 i = 0; i < m_num_application_tiles; i++)
   {
      m_local_clock_list[i] = 0;
      m_barrier_acquire_list[i] = false;
   }
}

LaxBarrierSyncServer::~LaxBarrierSyncServer()
{}

void
LaxBarrierSyncServer::processSyncMsg(core_id_t core_id)
{
   barrierWait(core_id);
}

void
LaxBarrierSyncServer::signal()
{
   if (isBarrierReached())
   {
     barrierRelease(); 
   }
}

void
LaxBarrierSyncServer::barrierWait(core_id_t core_id)
{
   UInt64 time_ns;
   m_recv_buff >> time_ns;

   LOG_PRINT("Received 'SIM_BARRIER_WAIT' from Core(%i, %i), Time(%llu)", core_id.tile_id, core_id.core_type, time_ns);

   LOG_ASSERT_ERROR(m_thread_manager->isCoreRunning(core_id) != INVALID_THREAD_ID || m_thread_manager->isCoreInitializing(core_id) != INVALID_THREAD_ID, "Thread on core(%i) is not running or initializing at time(%llu)", core_id, time_ns);

   if (time_ns < m_next_barrier_time)
   {
      LOG_PRINT("Sent 'SIM_BARRIER_RELEASE' immediately time(%llu), m_next_barrier_time(%llu)", time, m_next_barrier_time);
      // LOG_PRINT_WARNING("tile_id(%i), local_clock(%llu), m_next_barrier_time(%llu), m_barrier_interval(%llu)", tile_id, time, m_next_barrier_time, m_barrier_interval);
      unsigned int reply = LaxBarrierSyncClient::BARRIER_RELEASE;

      m_network.netSend(core_id, MCP_SYSTEM_RESPONSE_TYPE, (char*) &reply, sizeof(reply));
      return;
   }

   if (core_id.core_type == MAIN_CORE_TYPE)
   {
      m_local_clock_list[core_id.tile_id] = time_ns;
      m_barrier_acquire_list[core_id.tile_id] = true;
   }
   else
      LOG_ASSERT_ERROR(false, "Invalid core type!");
 
   signal(); 
}

bool
LaxBarrierSyncServer::isBarrierReached()
{
   bool single_thread_barrier_reached = false;

   // Check if all threads have reached the barrier
   // All least one thread must have (sync_time > m_next_barrier_time)
   for (tile_id_t tile_id = 0; tile_id < (tile_id_t) m_num_application_tiles; tile_id++)
   {
      if (m_local_clock_list[tile_id] < m_next_barrier_time)
      {
         if (m_thread_manager->isCoreRunning(tile_id) != INVALID_THREAD_ID)
         {
            // Thread Running on this core has not reached the barrier
            // Wait for it to sync
            return false;
         }
      }
      else
      {
         LOG_ASSERT_ERROR(m_thread_manager->isCoreRunning(tile_id) != INVALID_THREAD_ID  || m_thread_manager->isCoreInitializing(tile_id) != INVALID_THREAD_ID , "Thread on core(%i) is not running or initializing at local_clock(%llu), m_next_barrier_time(%llu)", tile_id, m_local_clock_list[tile_id], m_next_barrier_time);

         // At least one thread has reached the barrier
         single_thread_barrier_reached = true;
      }
   }

   return single_thread_barrier_reached;
}

void
LaxBarrierSyncServer::barrierRelease()
{
   LOG_PRINT("Sending 'BARRIER_RELEASE'");

   // All threads have reached the barrier
   // Advance m_next_barrier_time
   // Release the Barrier
   
   // If a thread cannot be resumed, we have to advance the sync 
   // time till a thread can be resumed. Then only, will we have 
   // forward progress

   bool thread_resumed = false;
   while (!thread_resumed)
   {
      m_next_barrier_time += m_barrier_interval;
      LOG_PRINT("m_next_barrier_time updated to (%llu)", m_next_barrier_time);

      for (tile_id_t tile_id = 0; tile_id < (tile_id_t) m_num_application_tiles; tile_id++)
      {
         if (m_local_clock_list[tile_id] < m_next_barrier_time)
         {
            // Check if this core was running. If yes, send a message to that core
            if (m_barrier_acquire_list[tile_id] == true)
            {
               LOG_ASSERT_ERROR(m_thread_manager->isCoreRunning(tile_id) != INVALID_THREAD_ID || m_thread_manager->isCoreInitializing(tile_id) != INVALID_THREAD_ID, "(%i) has acquired barrier, local_clock(%i), m_next_barrier_time(%llu), but not initializing or running", tile_id, m_local_clock_list[tile_id], m_next_barrier_time);

               unsigned int reply = LaxBarrierSyncClient::BARRIER_RELEASE;

               m_network.netSend(Tile::getMainCoreId(tile_id), MCP_SYSTEM_RESPONSE_TYPE, (char*) &reply, sizeof(reply));

               m_barrier_acquire_list[tile_id] = false;

               thread_resumed = true;
            }
         }
      }
   }

   // Notify Statistics thread about the global time
   if (Sim()->getStatisticsThread())
      Sim()->getStatisticsThread()->notify(m_next_barrier_time);
}
