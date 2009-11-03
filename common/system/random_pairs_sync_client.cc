#include "random_pairs_sync_client.h"
#include "simulator.h"
#include "config.h"
#include "packetize.h"
#include "network.h"
#include "log.h"

RandomPairsSyncClient::RandomPairsSyncClient(Core* core):
   _core(core),
   _last_sync_cycle_count(0),
   _quantum(0),
   _slack(0)
{
   LOG_ASSERT_ERROR(Sim()->getConfig()->getApplicationCores() >= 3, 
         "Number of Cores must be >= 3 if 'random_pairs' scheme is used");

   try
   {
      _slack = (UInt64) Sim()->getCfg()->getInt("clock_skew_minimization/random_pairs/slack");
      _quantum = (UInt64) Sim()->getCfg()->getInt("clock_skew_minimization/random_pairs/quantum");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read clock_skew_minimization/random_pairs variables from config file");
   }

   gettimeofday(&_start_wall_clock_time, NULL);
   _rand_num.seed(1);

   // Register Call-back
   _core->getNetwork()->registerCallback(CLOCK_SKEW_MINIMIZATION, ClockSkewMinimizationClientNetworkCallback, this);
}

RandomPairsSyncClient::~RandomPairsSyncClient()
{
   _core->getNetwork()->unregisterCallback(CLOCK_SKEW_MINIMIZATION);
}

// Called by network thread
void
RandomPairsSyncClient::netProcessSyncMsg(NetPacket& recv_pkt)
{
   UInt32 msg_type;
   UInt64 cycle_count;

   UnstructuredBuffer recv_buf;
   recv_buf << make_pair(recv_pkt.data, recv_pkt.length);
   
   recv_buf >> msg_type >> cycle_count;
   SyncMsg sync_msg(recv_pkt.sender, (SyncMsg::MsgType) msg_type, cycle_count);

   LOG_PRINT("Core(%i), SyncMsg[sender(%i), type(%u), time(%llu)]",
         _core->getId(), sync_msg.sender, sync_msg.type, sync_msg.cycle_count);

   _lock.acquire();

   // SyncMsg
   //  - sender
   //  - type (REQ,ACK,WAIT)
   //  - cycle_count
   // Called by the Network thread
   Core::State core_state = _core->getState();
   if (core_state != Core::RUNNING)
   {
      // I dont want to synchronize against a non-running core
      LOG_ASSERT_ERROR(sync_msg.type == SyncMsg::REQ,
            "sync_msg.type(%u)", sync_msg.type);

      LOG_PRINT("Core(%i) not RUNNING: Sending ACK", _core->getId());

      UnstructuredBuffer send_buf;
      send_buf << (UInt32) SyncMsg::ACK << (UInt64) 0;
      _core->getNetwork()->netSend(sync_msg.sender, CLOCK_SKEW_MINIMIZATION, send_buf.getBuffer(), send_buf.size());
   }
   else
   {
      // Thread is RUNNING on core
      // Network thread must process the random sync requests
      if (sync_msg.type == SyncMsg::REQ)
      {
         // This may generate some WAIT messages
         processSyncReq(sync_msg);
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

   _lock.release();
}

void
RandomPairsSyncClient::processSyncReq(SyncMsg& sync_msg)
{
   assert(sync_msg.cycle_count >= _slack);

   // I dont want to lock this, so I just try to read the cycle count
   // Even if this is an approximate value, this is OK
   UInt64 cycle_count = _core->getPerformanceModel()->getCycleCount();

   LOG_PRINT("Core(%i): Time(%llu), SyncReq[sender(%i), msg_type(%u), time(%llu)]", 
      _core->getId(), cycle_count, sync_msg.sender, sync_msg.type, sync_msg.cycle_count);

   // 3 possible scenarios
   if (cycle_count > (sync_msg.cycle_count + _slack))
   {
      // Wait till the other core reaches this one
      UnstructuredBuffer send_buf;
      send_buf << (UInt32) SyncMsg::ACK << (UInt64) 0;
      _core->getNetwork()->netSend(sync_msg.sender, CLOCK_SKEW_MINIMIZATION, send_buf.getBuffer(), send_buf.size());

      // Goto sleep for a few microseconds
      // Self generate a WAIT msg
      LOG_PRINT("Core(%i): WAIT: Time(%llu)", _core->getId(), cycle_count - sync_msg.cycle_count);

      SyncMsg wait_msg(_core->getId(), SyncMsg::WAIT, cycle_count - sync_msg.cycle_count);
      _msg_queue.push_back(wait_msg);
   }
   else if ((cycle_count <= (sync_msg.cycle_count + _slack)) && (cycle_count >= (sync_msg.cycle_count - _slack)))
   {
      // Both the cores are in sync (Good)
      UnstructuredBuffer send_buf;
      send_buf << (UInt32) SyncMsg::ACK << (UInt64) 0;
      _core->getNetwork()->netSend(sync_msg.sender, CLOCK_SKEW_MINIMIZATION, send_buf.getBuffer(), send_buf.size());
   }
   else if (cycle_count < (sync_msg.cycle_count - _slack))
   {
      // Double up and catch up. Meanwhile, ask the other core to wait
      UnstructuredBuffer send_buf;
      send_buf << (UInt32) SyncMsg::ACK << (UInt64) (sync_msg.cycle_count - cycle_count);
      _core->getNetwork()->netSend(sync_msg.sender, CLOCK_SKEW_MINIMIZATION, send_buf.getBuffer(), send_buf.size());
   }
   else
   {
      LOG_PRINT_ERROR("Strange cycle counts: cycle_count(%llu), sender(%i), sender_cycle_count(%llu)",
            cycle_count, sync_msg.sender, sync_msg.cycle_count);
   }
}

// Called by user thread
void
RandomPairsSyncClient::synchronize()
{
   if (_core->getState() == Core::WAKING_UP_STAGE1)
   {
      _core->setState(Core::WAKING_UP_STAGE2);
      return;
   }
   else if (_core->getState() == Core::WAKING_UP_STAGE2)
      _core->setState(Core::RUNNING);

   UInt64 cycle_count = _core->getPerformanceModel()->getCycleCount();
   assert(cycle_count >= _last_sync_cycle_count);

   if ((cycle_count - _last_sync_cycle_count) >= _quantum)
   {
      LOG_PRINT("Core(%i): Starting Synchronization: Time(%llu), LastSyncTime(%llu)", _core->getId(), cycle_count, _last_sync_cycle_count);

      _lock.acquire();

      _last_sync_cycle_count = (cycle_count / _quantum) * _quantum;

      // Send SyncMsg to another core
      sendRandomSyncMsg();

      // Wait for Acknowledgement and other Wait messages
      _cond.wait(_lock);

      UInt64 wait_time = userProcessSyncMsgList();

      _lock.release();

      LOG_PRINT("Wait Time (%llu)", wait_time);

      gotoSleep(wait_time);
   }
}

void
RandomPairsSyncClient::sendRandomSyncMsg()
{
   UInt64 cycle_count = _core->getPerformanceModel()->getCycleCount();
   
   UInt32 num_app_cores = Config::getSingleton()->getApplicationCores();
   SInt32 offset = 1 + (SInt32) _rand_num.next((Random::value_t) (((float)num_app_cores - 1) / 2));
   core_id_t receiver = (_core->getId() + offset) % num_app_cores;

   LOG_PRINT("Core(%i) Sending SyncReq to %i", _core->getId(), receiver);

   UnstructuredBuffer send_buf;
   send_buf << (UInt32) SyncMsg::REQ << cycle_count;
   _core->getNetwork()->netSend(receiver, CLOCK_SKEW_MINIMIZATION, send_buf.getBuffer(), send_buf.size());
}

UInt64
RandomPairsSyncClient::userProcessSyncMsgList()
{
   bool ack_present = false;
   UInt64 max_wait_time = 0;

   std::list<SyncMsg>::iterator it;
   for (it = _msg_queue.begin(); it != _msg_queue.end(); it++)
   {
      LOG_PRINT("Core(%i) Process Sync Msg List: SyncMsg[sender(%i), type(%u), wait_time(%llu)]", _core->getId(), (*it).sender, (*it).type, (*it).cycle_count);
      
      assert(((*it).type == SyncMsg::WAIT) || ((*it).type == SyncMsg::ACK));

      if ((*it).cycle_count >= max_wait_time)
         max_wait_time = (*it).cycle_count;
      if ((*it).type == SyncMsg::ACK)
         ack_present = true;
   }
   
   assert(ack_present);
   _msg_queue.clear();

   return max_wait_time;
}

void
RandomPairsSyncClient::gotoSleep(UInt64 sleep_time)
{
   if (sleep_time > 0)
   {
      LOG_PRINT("Core(%i) going to sleep", _core->getId());

      // Set the CoreState to 'STALLED'
      _core->setState(Core::STALLED);

      __attribute((__unused__)) UInt64 elapsed_simulated_time = _core->getPerformanceModel()->getCycleCount();
      __attribute((__unused__)) UInt64 elapsed_wall_clock_time = getElapsedWallClockTime();

      // elapsed_simulated_time, sleep_time - in cycles (of target architecture)
      // elapsed_wall_clock_time - in microseconds
      usleep(sleep_time * elapsed_wall_clock_time / elapsed_simulated_time);

      // Set the CoreState to 'RUNNING'
      _core->setState(Core::RUNNING);
      
      LOG_PRINT("Core(%i) woken up", _core->getId());
   }
}

UInt64
RandomPairsSyncClient::getElapsedWallClockTime()
{
   // Returns the elapsed time in microseconds
   struct timeval curr_wall_clock_time;
   gettimeofday(&curr_wall_clock_time, NULL);

   if (curr_wall_clock_time.tv_usec < _start_wall_clock_time.tv_usec)
   {
      curr_wall_clock_time.tv_usec += 1000000;
      curr_wall_clock_time.tv_sec -= 1;
   }
   return ((curr_wall_clock_time.tv_sec - _start_wall_clock_time.tv_sec) * 1000000 +
      (curr_wall_clock_time.tv_usec - _start_wall_clock_time.tv_usec));
}
