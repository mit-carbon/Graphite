#ifndef __RANDOM_PAIRS_SYNC_CLIENT_H__
#define __RANDOM_PAIRS_SYNC_CLIENT_H__

#include <sys/time.h>
#include <list>

#include "clock_skew_minimization_object.h"
#include "core.h"
#include "lock.h"
#include "cond.h"
#include "random.h"
#include "fixed_types.h"

class RandomPairsSyncClient : public ClockSkewMinimizationClient
{
   public:
      class SyncMsg
      {
      public:
         enum MsgType
         {
            REQ = 0,
            ACK,
            WAIT,
            NUM_MSG_TYPES
         };

         core_id_t sender;
         MsgType type;
         UInt64 cycle_count;
      
         SyncMsg(core_id_t sender, MsgType type, UInt64 cycle_count)
         {
            this->sender = sender;
            this->type = type;
            this->cycle_count = cycle_count;
         }
         ~SyncMsg() {}
      };

   private:
      // Data Fields
      Core* _core;
      
      UInt64 _last_sync_cycle_count;
      UInt64 _quantum;
      UInt64 _slack;
      float _sleep_fraction;

      struct timeval _start_wall_clock_time;

      std::list<SyncMsg> _msg_queue;
      Lock _lock;
      ConditionVariable _cond;

      Random _rand_num;

      // Called by user thread
      UInt64 userProcessSyncMsgList(void);
      void sendRandomSyncMsg();
      void gotoSleep(UInt64 sleep_time);
      UInt64 getElapsedWallClockTime(void);
     
      // Called by network thread 
      void processSyncReq(SyncMsg& sync_msg, bool sleeping);
      

   public:
      RandomPairsSyncClient(Core* core);
      ~RandomPairsSyncClient();

      // Called by user thread
      void synchronize(void);

      // Called by network thread
      void netProcessSyncMsg(NetPacket& recv_pkt);



};

#endif /* __RANDOM_PAIRS_SYNC_CLIENT_H__ */
