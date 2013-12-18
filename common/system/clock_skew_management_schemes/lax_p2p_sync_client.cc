#include "lax_p2p_sync_client.h"
#include "simulator.h"
#include "tile_manager.h"
#include "config.h"
#include "packetize.h"
#include "network.h"
#include "core.h"
#include "core_model.h"
#include "log.h"

UInt64 LaxP2PSyncClient::MAX_TIME = ((UInt64) 1) << 60;

LaxP2PSyncClient::LaxP2PSyncClient(Core* core):
   _core(core),
   _last_sync_time(0),
   _quantum(0),
   _slack(0),
   _sleep_fraction(0.0)
{
   LOG_ASSERT_ERROR(Sim()->getConfig()->getApplicationTiles() >= 3, 
         "Number of Cores must be >= 3 if 'lax_p2p' scheme is used");

   try
   {
      _slack = (UInt64) Sim()->getCfg()->getInt("clock_skew_management/lax_p2p/slack");
      _quantum = (UInt64) Sim()->getCfg()->getInt("clock_skew_management/lax_p2p/quantum");
      _sleep_fraction = Sim()->getCfg()->getFloat("clock_skew_management/lax_p2p/sleep_fraction");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read clock_skew_management/lax_p2p parameters from config file");
   }

   gettimeofday(&_start_wall_clock_time, NULL);
   
   // Register Call-back
   _core->getTile()->getNetwork()->registerCallback(CLOCK_SKEW_MANAGEMENT, ClockSkewManagementClientNetworkCallback, this);
}

LaxP2PSyncClient::~LaxP2PSyncClient()
{
   _core->getTile()->getNetwork()->unregisterCallback(CLOCK_SKEW_MANAGEMENT);
}

void 
LaxP2PSyncClient::enable()
{
   _enabled = true;
  
   assert(_last_sync_time == 0); 
   gettimeofday(&_start_wall_clock_time, NULL);
   assert(_msg_queue.empty());
}

void
LaxP2PSyncClient::disable()
{
   _enabled = false;
}

// Called by network thread
void
LaxP2PSyncClient::netProcessSyncMsg(const NetPacket& recv_pkt)
{
   UInt32 msg_type;
   UInt64 time;

   UnstructuredBuffer recv_buf;
   recv_buf << make_pair(recv_pkt.data, recv_pkt.length);
   
   recv_buf >> msg_type >> time;
   SyncMsg sync_msg(recv_pkt.sender, (SyncMsg::MsgType) msg_type, time);

   LOG_PRINT("Core(%i,%i), SyncMsg[sender(%i), type(%u), time(%llu)]",
         _core->getTile()->getId(), sync_msg.sender.tile_id, sync_msg.sender.core_type, sync_msg.type, sync_msg.time);

   LOG_ASSERT_ERROR(time < MAX_TIME, 
         "SyncMsg[sender(%i, %i), msg_type(%u), time(%llu)]",
         recv_pkt.sender.tile_id, recv_pkt.sender.core_type, msg_type, time);

   _lock.acquire();

   // SyncMsg
   //  - sender
   //  - type (REQ,ACK,WAIT)
   //  - time
   // Called by the Network thread
   Core::State core_state = _core->getState();
   if (core_state == Core::RUNNING)
   {
      // Thread is RUNNING on tile
      // Network thread must process the random sync requests
      if (sync_msg.type == SyncMsg::REQ)
      {
         // This may generate some WAIT messages
         processSyncReq(sync_msg, false);
      }
      else if (sync_msg.type == SyncMsg::ACK)
      {
         // sync_msg.type == SyncMsg::ACK with '0' or non-zero wait time
         _msg_queue.push_back(sync_msg);
         _cond.signal();
      }
      else
      {
         LOG_PRINT_ERROR("Unrecognized Sync Msg, type(%u) from(%i)", sync_msg.type, sync_msg.sender);
      }
   }
   else if (core_state == Core::SLEEPING)
   {
      LOG_ASSERT_ERROR(sync_msg.type == SyncMsg::REQ,
            "sync_msg.type(%u)", sync_msg.type);
      
      processSyncReq(sync_msg, true);
   }
   else
   {
      // I dont want to synchronize against a non-running tile
      LOG_ASSERT_ERROR(sync_msg.type == SyncMsg::REQ,
            "sync_msg.type(%u)", sync_msg.type);

      LOG_PRINT("Tile(%i) not RUNNING: Sending ACK", _core->getTile()->getId());

      UnstructuredBuffer send_buf;
      send_buf << (UInt32) SyncMsg::ACK << (UInt64) 0;
      _core->getTile()->getNetwork()->netSend(sync_msg.sender, CLOCK_SKEW_MANAGEMENT, send_buf.getBuffer(), send_buf.size());
   }

   _lock.release();
}

void
LaxP2PSyncClient::processSyncReq(const SyncMsg& sync_msg, bool sleeping)
{
   assert(sync_msg.time >= _slack);

   // I dont want to lock this, so I just try to read the cycle count
   // Even if this is an approximate value, this is OK
   
   // Tile Clock to Global clock conversion
   UInt64 curr_time = _core->getModel()->getCurrTime().toNanosec();

   LOG_ASSERT_ERROR(curr_time < MAX_TIME, "curr_time(%llu)", curr_time);

   LOG_PRINT("Core(%i, %i): Time(%llu), SyncReq[sender(%i), msg_type(%u), time(%llu)]", 
      _core->getId().tile_id, _core->getId().core_type, curr_time, sync_msg.sender, sync_msg.type, sync_msg.time);

   // 3 possible scenarios
   if (curr_time > (sync_msg.time + _slack))
   {
      // Wait till the other tile reaches this one
      UnstructuredBuffer send_buf;
      send_buf << (UInt32) SyncMsg::ACK << (UInt64) 0;
      _core->getTile()->getNetwork()->netSend(sync_msg.sender, CLOCK_SKEW_MANAGEMENT, send_buf.getBuffer(), send_buf.size());

      if (!sleeping)
      {
         // Goto sleep for a few microseconds
         // Self generate a WAIT msg
         LOG_PRINT("Core(%i, %i): WAIT: Time(%llu)", _core->getId().tile_id, _core->getId().core_type, curr_time - sync_msg.time);
         LOG_ASSERT_ERROR((curr_time - sync_msg.time) < MAX_TIME,
               "[>]: curr_time(%llu), sync_msg[sender(%i, %i), msg_type(%u), time(%llu)]", 
               curr_time, sync_msg.sender.tile_id, sync_msg.sender.core_type, sync_msg.type, sync_msg.time);

         SyncMsg wait_msg(_core->getId(), SyncMsg::WAIT, curr_time - sync_msg.time);
         _msg_queue.push_back(wait_msg);
      }
   }
   else if ((curr_time <= (sync_msg.time + _slack)) && (curr_time >= (sync_msg.time - _slack)))
   {
      // Both the cores are in sync (Good)
      UnstructuredBuffer send_buf;
      send_buf << (UInt32) SyncMsg::ACK << (UInt64) 0;
      _core->getTile()->getNetwork()->netSend(sync_msg.sender, CLOCK_SKEW_MANAGEMENT, send_buf.getBuffer(), send_buf.size());
   }
   else if (curr_time < (sync_msg.time - _slack))
   {
      LOG_ASSERT_ERROR((sync_msg.time - curr_time) < MAX_TIME,
            "[<]: curr_time(%llu), sync_msg[sender(%i), msg_type(%u), time(%llu)]",
            curr_time, sync_msg.sender, sync_msg.type, sync_msg.time);

      // Double up and catch up. Meanwhile, ask the other tile to wait
      UnstructuredBuffer send_buf;
      send_buf << (UInt32) SyncMsg::ACK << (UInt64) (sync_msg.time - curr_time);
      _core->getTile()->getNetwork()->netSend(sync_msg.sender, CLOCK_SKEW_MANAGEMENT, send_buf.getBuffer(), send_buf.size());
   }
   else
   {
      LOG_PRINT_ERROR("Strange cycle counts: curr_time(%llu), sender(%i), sender_time(%llu)",
            curr_time, sync_msg.sender, sync_msg.time);
   }
}

// Called by user thread
void
LaxP2PSyncClient::synchronize(Time time)
{
   LOG_ASSERT_ERROR(time == 0, "tiem(%llu), Cannot be used", time.toNanosec());

   if (! _enabled)
      return;

   if (_core->getState() == Core::WAKING_UP)
      _core->setState(Core::RUNNING);

   UInt64 curr_time = _core->getModel()->getCurrTime().toNanosec();

   assert(curr_time >= _last_sync_time);

   LOG_ASSERT_ERROR(curr_time < MAX_TIME, "curr_time(%llu)", curr_time);

   if ((curr_time - _last_sync_time) >= _quantum)
   {
      LOG_PRINT("Tile(%i): Starting Synchronization: curr_time(%llu), _last_sync_time(%llu)",
            _core->getTile()->getId(), curr_time, _last_sync_time);

      _lock.acquire();

      _last_sync_time = (curr_time / _quantum) * _quantum;

      LOG_ASSERT_ERROR(_last_sync_time < MAX_TIME,
            "_last_sync_time(%llu)", _last_sync_time);

      // Send SyncMsg to another tile
      sendRandomSyncMsg(curr_time);

      // Wait for Acknowledgement and other Wait messages
      _cond.wait(_lock);

      UInt64 wait_time = userProcessSyncMsgList();

      LOG_PRINT("Wait Time (%llu)", wait_time);

      gotoSleep(wait_time);
      
      _lock.release();

   }
}

void
LaxP2PSyncClient::sendRandomSyncMsg(UInt64 curr_time)
{
   LOG_ASSERT_ERROR(curr_time < MAX_TIME, "curr_time(%llu)", curr_time);

   UInt32 num_app_cores = Config::getSingleton()->getApplicationTiles();
   SInt32 offset = 1 + _rand_num.next((num_app_cores-1)/2);
   tile_id_t receiver = (_core->getTile()->getId() + offset) % num_app_cores;

   LOG_ASSERT_ERROR((receiver >= 0) && (receiver < (tile_id_t) num_app_cores), 
         "receiver(%i)", receiver);

   LOG_PRINT("Tile(%i) Sending SyncReq to %i, curr_time(%llu)", _core->getTile()->getId(), receiver, curr_time);

   UnstructuredBuffer send_buf;
   send_buf << (UInt32) SyncMsg::REQ << curr_time;

   _core->getTile()->getNetwork()->netSend(Tile::getMainCoreId(receiver), CLOCK_SKEW_MANAGEMENT, send_buf.getBuffer(), send_buf.size());
}

UInt64
LaxP2PSyncClient::userProcessSyncMsgList()
{
   bool ack_present = false;
   UInt64 max_wait_time = 0;

   std::list<SyncMsg>::iterator it;
   for (it = _msg_queue.begin(); it != _msg_queue.end(); it++)
   {
      LOG_PRINT("Tile(%i) Process Sync Msg List: SyncMsg[sender(%i), type(%u), wait_time(%llu)]", 
            _core->getTile()->getId(), (*it).sender, (*it).type, (*it).time);
      
      LOG_ASSERT_ERROR((*it).time < MAX_TIME,
            "sync_msg[sender(%i), msg_type(%u), time(%llu)]",
            (*it).sender, (*it).type, (*it).time);

      assert(((*it).type == SyncMsg::WAIT) || ((*it).type == SyncMsg::ACK));

      if ((*it).time >= max_wait_time)
         max_wait_time = (*it).time;
      if ((*it).type == SyncMsg::ACK)
         ack_present = true;
   }
   
   assert(ack_present);
   _msg_queue.clear();

   return max_wait_time;
}

void
LaxP2PSyncClient::gotoSleep(const UInt64 sleep_time)
{
   LOG_ASSERT_ERROR(sleep_time < MAX_TIME, "sleep_time(%llu)", sleep_time);

   if (sleep_time > 0)
   {
      LOG_PRINT("Tile(%i) going to sleep", _core->getTile()->getId());

      // Set the CoreState to 'SLEEPING'
      _core->setState(Core::SLEEPING);

      UInt64 elapsed_simulated_time = _core->getModel()->getCurrTime().toNanosec();

      UInt64 elapsed_wall_clock_time = getElapsedWallClockTime();

      // elapsed_simulated_time, sleep_time - in cycles (of target architecture)
      // elapsed_wall_clock_time - in microseconds
      assert(elapsed_simulated_time != 0);

      float wall_clock_time_per_simulated_cycle = float(elapsed_wall_clock_time) / elapsed_simulated_time;
      useconds_t sleep_wall_clock_time = (useconds_t) (_sleep_fraction * wall_clock_time_per_simulated_cycle * sleep_time);
      if (sleep_wall_clock_time > 1000000)
      {
         // LOG_PRINT_WARNING("Large Sleep Time(%u microseconds), SimSleep Time(%llu), elapsed_wall_clock_time(%llu), elapsed_simulated_time(%llu)", sleep_wall_clock_time, sleep_time, elapsed_wall_clock_time, elapsed_simulated_time);
         sleep_wall_clock_time = 1000000;
      }
     
      _lock.release();

      assert(usleep(sleep_wall_clock_time) == 0);

      _lock.acquire();

      // Set the CoreState to 'RUNNING'
      _core->setState(Core::RUNNING);
      
      LOG_PRINT("Tile(%i) woken up", _core->getTile()->getId());
   }
}

UInt64
LaxP2PSyncClient::getElapsedWallClockTime()
{
   // Returns the elapsed time in microseconds
   struct timeval curr_wall_clock_time;
   gettimeofday(&curr_wall_clock_time, NULL);

   if (curr_wall_clock_time.tv_usec < _start_wall_clock_time.tv_usec)
   {
      curr_wall_clock_time.tv_usec += 1000000;
      curr_wall_clock_time.tv_sec -= 1;
   }
   return ( ((UInt64) (curr_wall_clock_time.tv_sec - _start_wall_clock_time.tv_sec)) * 1000000 +
      (UInt64) (curr_wall_clock_time.tv_usec - _start_wall_clock_time.tv_usec));
}
