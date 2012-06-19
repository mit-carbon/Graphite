#pragma once

#include "mode.h"
#include "fixed_types.h"
#include "directory_state.h"

namespace HybridProtocol_PPMOSI_SS
{

// Forward Decls
class ShmemMsg;

class ShmemReq
{
public:
   ShmemReq(const ShmemMsg* msg, UInt64 time);
   ~ShmemReq();

   ShmemMsg* getShmemMsg() const
   { return _msg; }
   UInt64 getTime() const
   { return _time; }
   void setExpectedMsgSender(tile_id_t expected_msg_sender)
   { _expected_msg_sender = expected_msg_sender; }
   void setMode(Mode mode)
   { _mode = mode; }
   void updateTime(UInt64 time)
   { if (time > _time) _time = time; }

   void insertData(Byte* data, UInt32 size);
   Byte* lookupData();
   void eraseData();

   bool restartable(DirectoryState::Type dstate, tile_id_t last_msg_sender, ShmemMsg* last_msg);

private:
   ShmemMsg* _msg;
   UInt64 _time;
   Byte* _data_buf;
   Mode _mode;
   tile_id_t _expected_msg_sender;
};

}
