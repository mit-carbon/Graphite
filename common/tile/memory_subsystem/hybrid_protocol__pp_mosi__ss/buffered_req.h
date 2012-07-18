#pragma once

#include "mode.h"
#include "fixed_types.h"
#include "directory_entry.h"

namespace HybridProtocol_PPMOSI_SS
{

// Forward Decls
class ShmemMsg;

class BufferedReq
{
public:
   BufferedReq(const ShmemMsg* msg, UInt64 time);
   ~BufferedReq();

   ShmemMsg* getShmemMsg() const
   { return _msg; }
   UInt64 getTime() const
   { return _time; }
   Mode::Type getMode() const
   { return _mode; }
   void setExpectedMsgSender(tile_id_t expected_msg_sender)
   { _expected_msg_sender = expected_msg_sender; }
   void setMode(Mode::Type mode)
   { _mode = mode; }
   void updateTime(UInt64 time)
   { if (time > _time) _time = time; }

   void insertData(Byte* data, UInt32 size);
   Byte* lookupData();
   void eraseData();

   void setCacheLineDirty(bool cache_line_dirty)
   { _cache_line_dirty = cache_line_dirty; }
   bool isCacheLineDirty() const
   { return _cache_line_dirty; }

   bool restartable(DirectoryEntry* directory_entry, tile_id_t last_msg_sender, ShmemMsg* last_msg);

private:
   ShmemMsg* _msg;
   UInt64 _time;
   Byte* _data_buf;
   bool _cache_line_dirty;
   Mode::Type _mode;
   tile_id_t _expected_msg_sender;
};

}
