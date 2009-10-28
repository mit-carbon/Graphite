#include <cassert>

#include "simulation_barrier_client.h"
#include "simulator.h"
#include "config.h"
#include "message_types.h"
#include "packet_type.h"
#include "packetize.h"
#include "network.h"
#include "core.h"
#include "performance_model.h"

SimulationBarrierClient::SimulationBarrierClient(Core* core):
   m_core(core)
{
   try
   {
      m_barrier_interval = (UInt64) Sim()->getCfg()->getInt("simulation_barrier/interval"); 
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error Reading 'simulation_barrier/interval' from the config file");
   }
   m_next_sync_time = m_barrier_interval;
}

SimulationBarrierClient::~SimulationBarrierClient()
{}

void
SimulationBarrierClient::handlePeriodicBarrier()
{
   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   UInt64 curr_cycle_count = m_core->getPerformanceModel()->getCycleCount();
   if (curr_cycle_count >= m_next_sync_time)
   {
      // Send 'SIM_BARRIER_WAIT' request
      int msg_type = MCP_MESSAGE_SIM_BARRIER_WAIT;

      m_send_buff << msg_type << curr_cycle_count;
      m_core->getNetwork()->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_SYSTEM_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

      LOG_PRINT("Core(%i), m_cycle_count(%llu), m_next_sync_time(%llu) sent SIM_BARRIER_WAIT", m_core->getId(), curr_cycle_count, m_next_sync_time);

      // Receive 'SIM_BARRIER_RELEASE' response
      NetPacket recv_pkt;
      recv_pkt = m_core->getNetwork()->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_SYSTEM_RESPONSE_TYPE);
      assert(recv_pkt.length == sizeof(int));

      unsigned int dummy;
      m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
      m_recv_buff >> dummy;
      assert(dummy == SIM_BARRIER_RELEASE);

      LOG_PRINT("Core(%i) received SIM_BARRIER_RELEASE", m_core->getId());

      // Update 'm_next_sync_time'
      m_next_sync_time = ((curr_cycle_count / m_barrier_interval) * m_barrier_interval) + m_barrier_interval;

      // Delete the data buffer
      delete [] (Byte*) recv_pkt.data;
   }
}
