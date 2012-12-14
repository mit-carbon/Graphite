#include <string.h>
#include "shmem_msg.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMSI
{
   ShmemMsg::ShmemMsg()
      : _msg_type(INVALID_MSG_TYPE)
      , _sender_mem_component(MemComponent::INVALID)
      , _receiver_mem_component(MemComponent::INVALID)
      , _requester(INVALID_TILE_ID)
      , _address(INVALID_ADDRESS)
      , _data_buf(NULL)
      , _data_length(0)
      , _modeled(false)
   {}

   ShmemMsg::ShmemMsg(Type msg_type,
         MemComponent::Type sender_mem_component,
         MemComponent::Type receiver_mem_component,
         tile_id_t requester,
         IntPtr address,
         bool modeled)
      : _msg_type(msg_type)
      , _sender_mem_component(sender_mem_component)
      , _receiver_mem_component(receiver_mem_component)
      , _requester(requester)
      , _address(address)
      , _data_buf(NULL)
      , _data_length(0)
      , _modeled(modeled)
   {}

   ShmemMsg::ShmemMsg(Type msg_type,
         MemComponent::Type sender_mem_component,
         MemComponent::Type receiver_mem_component,
         tile_id_t requester,
         IntPtr address,
         Byte* data_buf,
         UInt32 data_length,
         bool modeled)
      : _msg_type(msg_type)
      , _sender_mem_component(sender_mem_component)
      , _receiver_mem_component(receiver_mem_component)
      , _requester(requester)
      , _address(address)
      , _data_buf(data_buf)
      , _data_length(data_length)
      , _modeled(modeled)
   {}

   ShmemMsg::ShmemMsg(const ShmemMsg* shmem_msg)
      : _msg_type(shmem_msg->getType())
      , _sender_mem_component(shmem_msg->getSenderMemComponent())
      , _receiver_mem_component(shmem_msg->getReceiverMemComponent())
      , _requester(shmem_msg->getRequester())
      , _address(shmem_msg->getAddress())
      , _data_buf(shmem_msg->getDataBuf())
      , _data_length(shmem_msg->getDataLength())
      , _modeled(shmem_msg->isModeled())
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

   UInt32
   ShmemMsg::getModeledLength()
   {
      switch(_msg_type)
      {
      case EX_REQ:
      case SH_REQ:
      case INV_REQ:
      case FLUSH_REQ:
      case WB_REQ:
      case UPGRADE_REP:
      case INV_REP:
         // msg_type + address
         return (_num_msg_type_bits + _num_physical_address_bits);
         
      case EX_REP:
      case SH_REP:
      case FLUSH_REP:
      case WB_REP:
         // msg_type + address + cache_block
         return (_num_msg_type_bits + _num_physical_address_bits + _data_length * 8);

      default:
         LOG_PRINT_ERROR("Unrecognized Msg Type(%u)", _msg_type);
         return 0;
      }
   }

}
