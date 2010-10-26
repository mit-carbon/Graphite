#include "sync_client.h"
#include "network.h"
#include "core.h"
#include "packetize.h"
#include "mcp.h"
#include "clock_converter.h"
#include "fxsupport.h"

#include <iostream>

using namespace std;

SyncClient::SyncClient(Tile *core)
      : m_core(core)
      , m_network(core->getNetwork())
{
}

SyncClient::~SyncClient()
{
}

void SyncClient::mutexInit(carbon_mutex_t *mux)
{
   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_MUTEX_INIT;

   m_send_buff << msg_type;

   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(carbon_mutex_t));

   *mux = *((carbon_mutex_t*)recv_pkt.data);

   delete [](Byte*) recv_pkt.data;
}

void SyncClient::mutexLock(carbon_mutex_t *mux)
{
   // Save/Restore Floating Point state
   FloatingPointHandler floating_point_handler;

   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_MUTEX_LOCK;

   // Tile Clock to Global Clock
   UInt64 start_time = convertCycleCount(m_core->getPerformanceModel()->getCycleCount(), \
         m_core->getPerformanceModel()->getFrequency(), 1.0);

   m_send_buff << msg_type << *mux << start_time;

   LOG_PRINT("mutexLock(): mux(%u), start_time(%llu)", *mux, start_time);
   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // Set the CoreState to 'STALLED'
   m_network->getCore()->setState(Tile::STALLED);

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(unsigned int) + sizeof(UInt64));

   // Set the CoreState to 'RUNNING'
   m_network->getCore()->setState(Tile::WAKING_UP);

   unsigned int dummy;
   UInt64 time;
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
   m_recv_buff >> dummy;
   assert(dummy == MUTEX_LOCK_RESPONSE);

   m_recv_buff >> time;

   if (time > start_time)
   {
      // Global Clock to Tile Clock
      UInt64 cycles_elapsed = convertCycleCount(time - start_time, \
            1.0, m_core->getPerformanceModel()->getFrequency());

      m_core->getPerformanceModel()->queueDynamicInstruction(new SyncInstruction(cycles_elapsed));
   }

   delete [](Byte*) recv_pkt.data;
}

void SyncClient::mutexUnlock(carbon_mutex_t *mux)
{
   // Save/Restore Floating Point state
   FloatingPointHandler floating_point_handler;

   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_MUTEX_UNLOCK;

   // Tile Clock to Global Clock
   UInt64 start_time = convertCycleCount(m_core->getPerformanceModel()->getCycleCount(), \
         m_core->getPerformanceModel()->getFrequency(), 1.0);

   m_send_buff << msg_type << *mux << start_time;

   LOG_PRINT("mutexUnlock(): mux(%u), start_time(%llu)", *mux, start_time);
   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(unsigned int));

   unsigned int dummy;
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
   m_recv_buff >> dummy;
   assert(dummy == MUTEX_UNLOCK_RESPONSE);

   delete [](Byte*) recv_pkt.data;
}

void SyncClient::condInit(carbon_cond_t *cond)
{
   // Save/Restore Floating Point state
   FloatingPointHandler floating_point_handler;

   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_COND_INIT;

   // Tile Clock to Global Clock
   UInt64 start_time = convertCycleCount(m_core->getPerformanceModel()->getCycleCount(), \
         m_core->getPerformanceModel()->getFrequency(), 1.0);

   m_send_buff << msg_type << *cond << start_time;

   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(carbon_cond_t));

   *cond = *((carbon_cond_t*)recv_pkt.data);

   delete [](Byte*) recv_pkt.data;
}

void SyncClient::condWait(carbon_cond_t *cond, carbon_mutex_t *mux)
{
   // Save/Restore Floating Point state
   FloatingPointHandler floating_point_handler;

   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_COND_WAIT;

   // Tile Clock to Global Clock
   UInt64 start_time = convertCycleCount(m_core->getPerformanceModel()->getCycleCount(), \
         m_core->getPerformanceModel()->getFrequency(), 1.0);

   m_send_buff << msg_type << *cond << *mux << start_time;

   LOG_PRINT("condWait(): cond(%u), mux(%u), start_time(%llu)", *cond, *mux, start_time);
   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // Set the CoreState to 'STALLED'
   m_network->getCore()->setState(Tile::STALLED);

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(unsigned int) + sizeof(UInt64));

   // Set the CoreState to 'RUNNING'
   m_network->getCore()->setState(Tile::WAKING_UP);

   unsigned int dummy;
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
   m_recv_buff >> dummy;
   assert(dummy == COND_WAIT_RESPONSE);

   UInt64 time;
   m_recv_buff >> time;

   if (time > start_time)
   {
      // Global Clock to Tile Clock
      UInt64 cycles_elapsed = convertCycleCount(time  - start_time, \
            1.0, m_core->getPerformanceModel()->getFrequency());

      m_core->getPerformanceModel()->queueDynamicInstruction(new SyncInstruction(cycles_elapsed));
   }

   delete [](Byte*) recv_pkt.data;
}

void SyncClient::condSignal(carbon_cond_t *cond)
{
   // Save/Restore Floating Point state
   FloatingPointHandler floating_point_handler;

   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_COND_SIGNAL;

   // Tile Clock to Global Clock
   UInt64 start_time = convertCycleCount(m_core->getPerformanceModel()->getCycleCount(), \
         m_core->getPerformanceModel()->getFrequency(), 1.0);

   m_send_buff << msg_type << *cond << start_time;

   LOG_PRINT("condSignal(): cond(%u), start_time(%llu)", *cond, start_time);
   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(unsigned int));

   unsigned int dummy;
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
   m_recv_buff >> dummy;
   assert(dummy == COND_SIGNAL_RESPONSE);

   delete [](Byte*) recv_pkt.data;
}

void SyncClient::condBroadcast(carbon_cond_t *cond)
{
   // Save/Restore Floating Point state
   FloatingPointHandler floating_point_handler;

   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_COND_BROADCAST;

   // Tile Clock to Global Clock
   UInt64 start_time = convertCycleCount(m_core->getPerformanceModel()->getCycleCount(), \
         m_core->getPerformanceModel()->getFrequency(), 1.0);

   m_send_buff << msg_type << *cond << start_time;

   LOG_PRINT("condBroadcast(): cond(%u), start_time(%llu)", *cond, start_time);
   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(unsigned int));

   unsigned int dummy;
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
   m_recv_buff >> dummy;
   assert(dummy == COND_BROADCAST_RESPONSE);

   delete [](Byte*) recv_pkt.data;
}

void SyncClient::barrierInit(carbon_barrier_t *barrier, UInt32 count)
{
   // Save/Restore Floating Point state
   FloatingPointHandler floating_point_handler;

   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_BARRIER_INIT;

   // Tile Clock to Global Clock
   UInt64 start_time = convertCycleCount(m_core->getPerformanceModel()->getCycleCount(), \
         m_core->getPerformanceModel()->getFrequency(), 1.0);

   m_send_buff << msg_type << count << start_time;

   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(carbon_barrier_t));

   *barrier = *((carbon_barrier_t*)recv_pkt.data);

   delete [](Byte*) recv_pkt.data;
}

void SyncClient::barrierWait(carbon_barrier_t *barrier)
{
   // Save/Restore Floating Point state
   FloatingPointHandler floating_point_handler;

   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_BARRIER_WAIT;

   // Tile Clock to Global Clock
   UInt64 start_time = convertCycleCount(m_core->getPerformanceModel()->getCycleCount(), \
         m_core->getPerformanceModel()->getFrequency(), 1.0);

   m_send_buff << msg_type << *barrier << start_time;

   LOG_PRINT("barrierWait(): barrier(%u), start_time(%llu)", *barrier, start_time);
   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // Set the CoreState to 'STALLED'
   m_network->getCore()->setState(Tile::STALLED);

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(unsigned int) + sizeof(UInt64));

   // Set the CoreState to 'RUNNING'
   m_network->getCore()->setState(Tile::WAKING_UP);

   unsigned int dummy;
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
   m_recv_buff >> dummy;
   assert(dummy == BARRIER_WAIT_RESPONSE);

   UInt64 time;
   m_recv_buff >> time;

   if (time > start_time)
   {
      // Global Clock to Tile Clock
      UInt64 cycles_elapsed = convertCycleCount(time - start_time, \
            1.0, m_core->getPerformanceModel()->getFrequency());

      m_core->getPerformanceModel()->queueDynamicInstruction(new SyncInstruction(cycles_elapsed));
   }

   delete [](Byte*) recv_pkt.data;
}
