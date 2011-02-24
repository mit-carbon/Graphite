#ifndef __RANDOM_PAIRS_SYNC_CLIENT_H__
#define __RANDOM_PAIRS_SYNC_CLIENT_H__

#include <sys/time.h>
#include <list>

#include "clock_skew_minimization_object.h"
#include "tile.h"
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

         //tile_id_t sender;
         core_id_t sender;
         MsgType type;
         UInt64 time;
      
         SyncMsg(core_id_t sender, MsgType type, UInt64 time)
         {
            this->sender = sender;
            this->type = type;
            this->time = time;
         }
         ~SyncMsg() {}
      };

   private:
      // Data Fields
      Core* _core;
      
      UInt64 _last_sync_time;
      UInt64 _quantum;
      UInt64 _slack;
      volatile float _sleep_fraction;

      struct timeval _start_wall_clock_time;

      std::list<SyncMsg> _msg_queue;
      Lock _lock;
      ConditionVariable _cond;
      Random _rand_num;

      bool _enabled;

      static UInt64 MAX_TIME;

      // Called by user thread
      UInt64 userProcessSyncMsgList(void);
      void sendRandomSyncMsg(UInt64 curr_time);
      void gotoSleep(const UInt64 sleep_time);
      UInt64 getElapsedWallClockTime(void);
     
      // Called by network thread 
      void processSyncReq(const SyncMsg& sync_msg, bool sleeping);
      

   public:
      RandomPairsSyncClient(Core* core);
      ~RandomPairsSyncClient();

      void enable();
      void disable();
      void reset();

      // Called by user thread
      void synchronize(UInt64 cycle_count);

      // Called by network thread
      void netProcessSyncMsg(const NetPacket& recv_pkt);



};

#endif /* __RANDOM_PAIRS_SYNC_CLIENT_H__ */
