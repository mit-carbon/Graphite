#include <cstring>
#include <cassert>
#include <cmath>
#include "shmem_msg.h"
#include "utils.h"
#include "log.h"

namespace ShL1ShL2
{
   UInt32 ShmemMsg::_metadata_size;

   ShmemMsg::ShmemMsg()
      : _msg_type(INVALID_MSG_TYPE)
      , _sender_mem_component(MemComponent::INVALID)
      , _receiver_mem_component(MemComponent::INVALID)
      , _requester(INVALID_TILE_ID)
      , _address(INVALID_ADDRESS)
      , _offset(0)
      , _size(0)
      , _data_buf(NULL)
      , _data_length(0)
   {}

   ShmemMsg::ShmemMsg(Type msg_type,
                      MemComponent::Type sender_mem_component,
                      MemComponent::Type receiver_mem_component,
                      tile_id_t requester,
                      IntPtr address,
                      UInt32 offset,
                      UInt32 size,
                      Byte* data_buf,
                      UInt32 data_length)
      : _msg_type(msg_type)
      , _sender_mem_component(sender_mem_component)
      , _receiver_mem_component(receiver_mem_component)
      , _requester(requester)
      , _address(address)
      , _offset(offset)
      , _size(size)
      , _data_buf(data_buf)
      , _data_length(data_length)
   {}

   ShmemMsg::~ShmemMsg()
   {}

   ShmemMsg*
   ShmemMsg::getShmemMsg(Byte* msg_buf)
   {
      ShmemMsg* shmem_msg = new ShmemMsg();
      memcpy((void*) shmem_msg, msg_buf, sizeof(*shmem_msg));
      if (shmem_msg->getDataLength() > 0)
      {
         shmem_msg->setDataBuf(new Byte[shmem_msg->getDataLength()]);
         memcpy((void*) shmem_msg->getDataBuf(), msg_buf + sizeof(*shmem_msg), shmem_msg->getDataLength());
      }
      return shmem_msg;
   }

   Byte*
   ShmemMsg::makeMsgBuf()
   {
      Byte* msg_buf = new Byte[getMsgLen()];
      memcpy(msg_buf, (void*) this, sizeof(*this));
      if (_data_length > 0)
      {
         LOG_ASSERT_ERROR(_data_buf != NULL, "_data_buf(%p)", _data_buf);
         memcpy(msg_buf + sizeof(*this), (void*) _data_buf, _data_length); 
      }

      return msg_buf; 
   }

   UInt32
   ShmemMsg::getMsgLen()
   {
      return (sizeof(*this) + _data_length);
   }

   ShmemMsg*
   ShmemMsg::clone() const
   {
      ShmemMsg* shmem_msg = new ShmemMsg();
      memcpy(shmem_msg, this, sizeof(*this));
      if (_data_length > 0)
      {
         shmem_msg->setDataBuf(new Byte[_data_length]);
         memcpy(shmem_msg->getDataBuf(), _data_buf, _data_length);
      }
      return shmem_msg;
   }

   void
   ShmemMsg::release()
   {
      if (_data_length > 0)
      {
         assert(_data_buf);
         delete [] _data_buf;
      }
      delete this;
   }

   UInt32
   ShmemMsg::getModeledLength() const
   {
      switch(_msg_type)
      {
      case READ_REQ:
      case READ_EX_REQ:
      case WRITE_REP:
         // Address[sizeof(IntPtr)], Size[log2(cache_line_size)], Msg Type[3]
         return _metadata_size;

      case WRITE_REQ:
      case READ_REP:
         // Address[sizeof(IntPtr)], Size[log2(cache_line_size)], Msg Type[3], Data[variable]
         return _metadata_size + _data_length;

      default:
         LOG_PRINT_ERROR("Unrecognized Msg Type(%u)", _msg_type);
         return 0;
      }
   }

   void
   ShmemMsg::setCacheLineSize(UInt32 cache_line_size)
   {
      UInt32 log2_cache_line_size = ceilLog2(cache_line_size);
      _metadata_size = (UInt32) (ceil(1.0 * (3 + log2_cache_line_size) / 8) + sizeof(IntPtr));
   }
}
