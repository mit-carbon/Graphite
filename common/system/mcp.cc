#include <syscall.h>
#include <sched.h>
#include <iostream>

#include "mcp.h"
#include "config.h"
#include "tile_manager.h"
#include "log.h"
#include "tile.h"
#include "simulator.h"
#include "syscall.h"
#include "thread_manager.h"
#include "thread_scheduler.h"

using namespace std;

MCP::MCP(Network & network)
   : m_finished(false)
   , m_network(network)
   , m_MCP_SERVER_MAX_BUFF(256*1024)
   , m_scratch(new char[m_MCP_SERVER_MAX_BUFF])
   , m_vm_manager()
   , m_syscall_server(m_network, m_send_buff, m_recv_buff, m_MCP_SERVER_MAX_BUFF, m_scratch)
   , m_sync_server(m_network, m_recv_buff)
   , m_clock_skew_management_server(NULL)
{
   m_clock_skew_management_server = ClockSkewManagementServer::create(Sim()->getCfg()->getString("clock_skew_management/scheme"), m_network, m_recv_buff);
}

MCP::~MCP()
{
   if (m_clock_skew_management_server)
      delete m_clock_skew_management_server;
   delete [] m_scratch;
}

void MCP::processPacket()
{
   m_send_buff.clear();
   m_recv_buff.clear();

   NetPacket recv_pkt;

   NetMatch match;
   match.types.push_back(MCP_REQUEST_TYPE);
   match.types.push_back(MCP_SYSTEM_TYPE);
   recv_pkt = m_network.netRecv(match);

   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int msg_type;

   m_recv_buff >> msg_type;

   LOG_PRINT("MCP message type(%i), sender(%i,%i)", (SInt32) msg_type, recv_pkt.sender.tile_id, recv_pkt.sender.core_type);

   switch (msg_type)
   {
   case MCP_MESSAGE_SYS_CALL:
      m_syscall_server.handleSyscall(recv_pkt.sender);
      break;
   case MCP_MESSAGE_QUIT:
      LOG_PRINT("Quit message received.");
      m_finished = true;
      break;

   case MCP_MESSAGE_MUTEX_INIT:
      m_sync_server.mutexInit(recv_pkt.sender);
      break;
   case MCP_MESSAGE_MUTEX_LOCK:
      m_sync_server.mutexLock(recv_pkt.sender);
      break;
   case MCP_MESSAGE_MUTEX_UNLOCK:
      m_sync_server.mutexUnlock(recv_pkt.sender);
      break;

   case MCP_MESSAGE_COND_INIT:
      m_sync_server.condInit(recv_pkt.sender);
      break;
   case MCP_MESSAGE_COND_WAIT:
      m_sync_server.condWait(recv_pkt.sender);
      break;
   case MCP_MESSAGE_COND_SIGNAL:
      m_sync_server.condSignal(recv_pkt.sender);
      break;
   case MCP_MESSAGE_COND_BROADCAST:
      m_sync_server.condBroadcast(recv_pkt.sender);
      break;

   case MCP_MESSAGE_BARRIER_INIT:
      m_sync_server.barrierInit(recv_pkt.sender);
      break;
   case MCP_MESSAGE_BARRIER_WAIT:
      m_sync_server.barrierWait(recv_pkt.sender);
      break;

   case MCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_REQUESTER:
      Sim()->getThreadManager()->masterSpawnThread((ThreadSpawnRequest*)recv_pkt.data);
      break;
   case MCP_MESSAGE_THREAD_SPAWN_REPLY_FROM_SLAVE:
      Sim()->getThreadManager()->masterSpawnThreadReply((ThreadSpawnRequest*)recv_pkt.data);
      break;
   case MCP_MESSAGE_THREAD_YIELD_REQUEST:
      Sim()->getThreadScheduler()->masterYieldThread((ThreadYieldRequest*)recv_pkt.data);
      break;
   case MCP_MESSAGE_THREAD_MIGRATE_REQUEST_FROM_REQUESTER:
      Sim()->getThreadScheduler()->masterMigrateThread( *(SInt32*)((Byte*)recv_pkt.data+sizeof(msg_type)), 
                                                        *(tile_id_t*)((Byte*)recv_pkt.data+sizeof(msg_type)+sizeof(SInt32)), 
                                                        *(UInt32*)((Byte*)recv_pkt.data+sizeof(msg_type)+sizeof(SInt32)+sizeof(tile_id_t))); 
      break;
   case MCP_MESSAGE_THREAD_SETAFFINITY_REQUEST:
      Sim()->getThreadScheduler()->masterSchedSetAffinity((ThreadAffinityRequest*)recv_pkt.data);
      break;
   case MCP_MESSAGE_THREAD_GETAFFINITY_REQUEST:
      Sim()->getThreadScheduler()->masterSchedGetAffinity((ThreadAffinityRequest*)recv_pkt.data);
      break;

   case MCP_MESSAGE_THREAD_SET_OS_TID:
      Sim()->getThreadManager()->masterSetOSTid(*(tile_id_t*)((Byte*)recv_pkt.data+sizeof(msg_type)), 
                                                *(thread_id_t*)((Byte*)recv_pkt.data+sizeof(msg_type)+sizeof(tile_id_t)), 
                                                *(pid_t*)((Byte*)recv_pkt.data+sizeof(msg_type)+sizeof(tile_id_t)+sizeof(thread_id_t)));
      break;

   case MCP_MESSAGE_QUERY_THREAD_INDEX:
      Sim()->getThreadManager()->masterQueryThreadIndex( *(tile_id_t*)((Byte*)recv_pkt.data+sizeof(msg_type)), 
                                                         *(UInt32*)((Byte*)recv_pkt.data+sizeof(msg_type)+sizeof(tile_id_t)), 
                                                         *(thread_id_t*)((Byte*)recv_pkt.data+sizeof(msg_type)+sizeof(tile_id_t)+sizeof(UInt32)));
      break;
   case MCP_MESSAGE_THREAD_START:
      Sim()->getThreadManager()->masterOnThreadStart( *(tile_id_t*)((Byte*)recv_pkt.data+sizeof(msg_type)), 
                                                      *(UInt32*)((Byte*)recv_pkt.data+sizeof(msg_type)+sizeof(tile_id_t)), 
                                                      *(SInt32*)((Byte*)recv_pkt.data+sizeof(msg_type)+sizeof(tile_id_t)+sizeof(UInt32)));

      break;
   case MCP_MESSAGE_THREAD_EXIT:
      Sim()->getThreadManager()->masterOnThreadExit(  *(tile_id_t*)((Byte*)recv_pkt.data+sizeof(msg_type)), 
                                                      *(UInt32*)((Byte*)recv_pkt.data+sizeof(msg_type)+sizeof(tile_id_t)), 
                                                      *(SInt32*)((Byte*)recv_pkt.data+sizeof(msg_type)+sizeof(tile_id_t)+sizeof(UInt32)), 
                                                      recv_pkt.time.getTime());
      break;

   case MCP_MESSAGE_THREAD_JOIN_REQUEST:
      Sim()->getThreadManager()->masterJoinThread((ThreadJoinRequest*)recv_pkt.data, recv_pkt.time);
      break;

   case MCP_MESSAGE_CLOCK_SKEW_MANAGEMENT:
      assert(m_clock_skew_management_server);
      m_clock_skew_management_server->processSyncMsg(recv_pkt.sender);
      break;

   default:
      LOG_PRINT_ERROR("Unhandled MCP message type: %i from %i", msg_type, recv_pkt.sender);
   }

   delete [](Byte*)recv_pkt.data;

   LOG_PRINT("Finished processing message -- type : %d", (int)msg_type);
}

void MCP::finish()
{
   LOG_PRINT("Send MCP quit message");

   SInt32 msg_type = MCP_MESSAGE_QUIT;
   m_network.netSend(Config::getSingleton()->getMCPCoreId(), MCP_SYSTEM_TYPE, &msg_type, sizeof(msg_type));

   while (!finished())
   {
      sched_yield();
   }

   LOG_PRINT("MCP Finished.");
}

void MCP::run()
{
   __attribute__((unused)) int tid =  syscall(__NR_gettid);
   LOG_PRINT("In MCP thread ... initializing thread (%i) with id: %i", (int)tid, Config::getSingleton()->getMCPTileNum());

   core_id_t mcp_core_id = Config::getSingleton()->getMCPCoreId();
   Sim()->getTileManager()->initializeThread(mcp_core_id);
   Sim()->getTileManager()->initializeCommId(mcp_core_id.tile_id);

   while (!finished())
   {
      processPacket();
   }
}

