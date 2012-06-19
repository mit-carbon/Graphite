#include <cstring>
#include "shmem_req.h"
#include "shmem_msg.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

ShmemReq::ShmemReq(const ShmemMsg* msg, UInt64 time)
   : _msg(new ShmemMsg(msg))
   , _time(time)
   , _data_buf(NULL)
   , _mode(INVALID_MODE)
   , _expected_msg_sender(INVALID_TILE_ID)
{
   LOG_ASSERT_ERROR(!_msg->getDataBuf(), "Directory Requests should not have data payloads");
}

ShmemReq::~ShmemReq()
{
   delete _msg;
   LOG_ASSERT_ERROR(!_data_buf, "Data Buf still allocated");
}

void
ShmemReq::insertData(Byte* data, UInt32 size)
{
   LOG_ASSERT_ERROR(!_data_buf, "Data buf ALREADY allocated");
   _data_buf = new Byte[size];
   memcpy(_data_buf, data, size);
}

Byte*
ShmemReq::lookupData()
{
   return _data_buf;
}

void
ShmemReq::eraseData()
{
   LOG_ASSERT_ERROR(_data_buf, "Data buf NOT allocated");
   delete [] _data_buf;
   _data_buf = NULL;
}

bool
ShmemReq::restartable(DirectoryState::Type dstate, tile_id_t last_msg_sender, ShmemMsg* last_msg)
{
   if (last_msg->getSenderMemComponent() == MemComponent::DRAM_CNTLR)
      return true;

   switch (_msg->getType())
   {
   case ShmemMsg::UNIFIED_READ_REQ:
      if (_mode == REMOTE_MODE)
      {
         return (dstate == DirectoryState::UNCACHED);
      }
      else // (_mode = PRIVATE_MODE, PRIVATE_SHARER_MODE, REMOTE_SHARER_MODE)
      {
         LOG_ASSERT_ERROR(_expected_msg_sender != INVALID_TILE_ID, "_expected_msg_sender(INVALID_TILE_ID)");
         bool match = (_expected_msg_sender == last_msg_sender);
         if (match)
            _expected_msg_sender = INVALID_TILE_ID;
         return match;
      }

   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
      return (dstate == DirectoryState::UNCACHED);

   default:
      LOG_PRINT_ERROR("Unrecognized type(%u)", _msg->getType());
      return false;
   }
}

}
