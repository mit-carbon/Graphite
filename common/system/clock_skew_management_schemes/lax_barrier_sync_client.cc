#include <cassert>

#include "tile.h"
#include "lax_barrier_sync_client.h"
#include "simulator.h"
#include "config.h"
#include "message_types.h"
#include "packet_type.h"
#include "packetize.h"
#include "network.h"
#include "core.h"
#include "core_model.h"

LaxBarrierSyncClient::LaxBarrierSyncClient(Core* core):
   m_core(core)
{
   try
   {
      m_barrier_interval = (UInt64) Sim()->getCfg()->getInt("clock_skew_management/lax_barrier/quantum"); 
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error Reading 'clock_skew_management/barrier/quantum' from the config file");
   }
   m_next_sync_time = m_barrier_interval;
}

LaxBarrierSyncClient::~LaxBarrierSyncClient()
{}

void
LaxBarrierSyncClient::synchronize(Time time)
{
   UnstructuredBuffer m_send_buff;
   UnstructuredBuffer m_recv_buff;

   UInt64 curr_time_ns = time.toNanosec();
   if (curr_time_ns == 0)
      curr_time_ns = m_core->getModel()->getCurrTime().toNanosec();

   if (curr_time_ns >= m_next_sync_time)
   {
      // Send 'SIM_BARRIER_WAIT' request
      int msg_type = MCP_MESSAGE_CLOCK_SKEW_MANAGEMENT;

      m_send_buff << msg_type << curr_time_ns;
      m_core->getTile()->getNetwork()->netSend(Config::getSingleton()->getMCPCoreId(), MCP_SYSTEM_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

      LOG_PRINT("Core(%i, %i), curr_time(%llu), m_next_sync_time(%llu) sent SIM_BARRIER_WAIT", m_core->getId().tile_id, m_core->getId().core_type, curr_time_ns, m_next_sync_time);

      // Receive 'BARRIER_RELEASE' response
      NetPacket recv_pkt;
      recv_pkt = m_core->getTile()->getNetwork()->netRecv(Config::getSingleton()->getMCPCoreId(), m_core->getId(), MCP_SYSTEM_RESPONSE_TYPE);
      assert(recv_pkt.length == sizeof(int));

      unsigned int dummy;
      m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
      m_recv_buff >> dummy;
      assert(dummy == BARRIER_RELEASE);

      LOG_PRINT("Tile(%i) received SIM_BARRIER_RELEASE", m_core->getTile()->getId());

      // Update 'm_next_sync_time'
      m_next_sync_time = ((curr_time_ns / m_barrier_interval) * m_barrier_interval) + m_barrier_interval;

      // Delete the data buffer
      delete [] (Byte*) recv_pkt.data;
   }
}
