#pragma once

#include <sys/time.h>
#include <list>

#include "clock_skew_management_object.h"
#include "tile.h"
#include "lock.h"
#include "cond.h"
#include "random.h"
#include "fixed_types.h"

class LaxP2PSyncClient : public ClockSkewManagementClient
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
   float _sleep_fraction;

   struct timeval _start_wall_clock_time;

   std::list<SyncMsg> _msg_queue;
   Lock _lock;
   ConditionVariable _cond;
   Random<int> _rand_num;

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
   LaxP2PSyncClient(Core* core);
   ~LaxP2PSyncClient();

   void enable();
   void disable();

   // Called by user thread
   void synchronize(Time time);

   // Called by network thread
   void netProcessSyncMsg(const NetPacket& recv_pkt);
};
