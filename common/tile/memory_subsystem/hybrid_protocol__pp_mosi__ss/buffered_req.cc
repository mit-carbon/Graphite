#include <cstring>
#include <cassert>
#include "buffered_req.h"
#include "shmem_msg.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

BufferedReq::BufferedReq(const ShmemMsg* msg, UInt64 time)
   : _msg(new ShmemMsg(msg))
   , _time(time)
   , _data_buf(NULL)
   , _cache_line_dirty(false)
   , _mode(Mode::INVALID)
   , _expected_msg_sender(INVALID_TILE_ID)
{
   LOG_PRINT("Ctor: Address(%#lx)", _msg->getAddress());
   assert((msg->getDataBuf() == NULL) == (msg->getDataBufSize() == 0));
   if (msg->getDataBuf())
   {
      Byte* data_buf = new Byte[msg->getDataBufSize()];
      memcpy(data_buf, msg->getDataBuf(), msg->getDataBufSize());
      _msg->setDataBuf(data_buf);
   }
}

BufferedReq::~BufferedReq()
{
   LOG_PRINT("Dtor: Address(%#lx)", _msg->getAddress());
   assert((_msg->getDataBuf() == NULL) == (_msg->getDataBufSize() == 0));
   if (_msg->getDataBuf())
      delete [] _msg->getDataBuf();
   delete _msg;
   LOG_ASSERT_ERROR(!_data_buf, "Data Buf still allocated");
}

void
BufferedReq::insertData(Byte* data, UInt32 size)
{
   LOG_PRINT("Insert: Address(%#lx), Size(%u)", _msg->getAddress(), size);
   LOG_ASSERT_ERROR(!_data_buf, "Data buf ALREADY allocated, MsgType(%s), Requester(%i)",
                    SPELL_SHMSG(_msg->getType()), _msg->getRequester());
   _data_buf = new Byte[size];
   memcpy(_data_buf, data, size);
}

Byte*
BufferedReq::lookupData()
{
   LOG_PRINT("Lookup: Address(%#lx)", _msg->getAddress());
   return _data_buf;
}

void
BufferedReq::eraseData()
{
   LOG_PRINT("Erase: Address(%#lx)", _msg->getAddress());
   LOG_ASSERT_ERROR(_data_buf, "Data buf NOT allocated");
   delete [] _data_buf;
   _data_buf = NULL;
}

bool
BufferedReq::restartable(DirectoryEntry* directory_entry, tile_id_t last_msg_sender, ShmemMsg* last_msg)
{
   if (last_msg->getSenderMemComponent() == MemComponent::DRAM_CNTLR)
      return true;

   if (_expected_msg_sender != INVALID_TILE_ID)
   {
      if (last_msg_sender == _expected_msg_sender)
      {
         _expected_msg_sender = INVALID_TILE_ID;
         return true;
      }
      return false;
   }

   DirectoryState::Type dstate = directory_entry->getDirectoryBlockInfo()->getDState();
   return (dstate == DirectoryState::UNCACHED);
}

}
