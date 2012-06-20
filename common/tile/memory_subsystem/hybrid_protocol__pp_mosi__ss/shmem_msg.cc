#include <cstring>
#include "shmem_msg.h"
#include "config.h"
#include "utils.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

UInt32 ShmemMsg::_num_msg_type_bits;
UInt32 ShmemMsg::_num_physical_address_bits;
UInt32 ShmemMsg::_cache_line_size;
UInt32 ShmemMsg::_num_data_length_bits;
UInt32 ShmemMsg::_num_tile_id_bits;

ShmemMsg::ShmemMsg()
{
   initialize();
}

ShmemMsg::ShmemMsg(Type type,
                   MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
                   IntPtr address,
                   tile_id_t requester, bool modeled)
{
   initialize();
   _type = type;
   _sender_mem_component = sender_mem_component;
   _receiver_mem_component = receiver_mem_component;
   _address = address;
   _requester = requester;
   _modeled = modeled;
}

ShmemMsg::ShmemMsg(Type type,
                   MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
                   IntPtr address,
                   Byte* data_buf, UInt32 data_buf_size,
                   tile_id_t requester, bool modeled)
{
   initialize();
   _type = type;
   _sender_mem_component = sender_mem_component;
   _receiver_mem_component = receiver_mem_component;
   _address = address;
   _data_buf = data_buf;
   _data_buf_size = data_buf_size;
   _requester = requester;
   _modeled = modeled;
}

ShmemMsg::ShmemMsg(Type type,
                   MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
                   IntPtr address,
                   UInt32 offset, UInt32 data_length,
                   tile_id_t requester, bool modeled)
{
   initialize();
   _type = type;
   _sender_mem_component = sender_mem_component;
   _receiver_mem_component = receiver_mem_component;
   _address = address;
   _offset = offset;
   _data_length = data_length;
   _requester = requester;
   _modeled = modeled;
}

ShmemMsg::ShmemMsg(Type type,
                   MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
                   IntPtr address,
                   UInt32 offset, UInt32 data_length,
                   Byte* data_buf, UInt32 data_buf_size,
                   tile_id_t requester, bool modeled)
{
   initialize();
   _type = type;
   _sender_mem_component = sender_mem_component;
   _receiver_mem_component = receiver_mem_component;
   _address = address;
   _offset = offset;
   _data_length = data_length;
   _data_buf = data_buf;
   _data_buf_size = data_buf_size;
   _requester = requester;
   _modeled = modeled;
}

ShmemMsg::ShmemMsg(Type type,
                   MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
                   IntPtr address,
                   tile_id_t single_receiver, bool reply_expected,
                   tile_id_t requester, bool modeled)
{
   initialize();
   _type = type;
   _sender_mem_component = sender_mem_component;
   _receiver_mem_component = receiver_mem_component;
   _address = address;
   _single_receiver = single_receiver;
   _reply_expected = reply_expected;
   _requester = requester;
   _modeled = modeled;
}

ShmemMsg::ShmemMsg(Type type,
                   MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
                   IntPtr address,
                   Byte* data_buf, UInt32 data_buf_size,
                   tile_id_t single_receiver, bool reply_expected,
                   tile_id_t requester, bool modeled)
{
   initialize();
   _type = type;
   _sender_mem_component = sender_mem_component;
   _receiver_mem_component = receiver_mem_component;
   _address = address;
   _data_buf = data_buf;
   _data_buf_size = data_buf_size;
   _single_receiver = single_receiver;
   _reply_expected = reply_expected;
   _requester = requester;
   _modeled = modeled;
}

void
ShmemMsg::initialize()
{
   _type = INVALID;
   _sender_mem_component = MemComponent::INVALID;
   _receiver_mem_component = MemComponent::INVALID;
   _address = INVALID_ADDRESS;
   _offset = 0;
   _data_length = 0;
   _data_buf = NULL;
   _data_buf_size = 0;
   _single_receiver = INVALID_TILE_ID;
   _reply_expected = false;
   _requester = INVALID_TILE_ID;
   _modeled = false;
}

ShmemMsg::ShmemMsg(const ShmemMsg* shmem_msg)
{
   assign(shmem_msg);
}

ShmemMsg::~ShmemMsg()
{}

void
ShmemMsg::assign(const ShmemMsg* shmem_msg)
{
   _type = shmem_msg->getType();
   _sender_mem_component = shmem_msg->getSenderMemComponent();
   _receiver_mem_component = shmem_msg->getReceiverMemComponent();
   _address = shmem_msg->getAddress();
   _offset = shmem_msg->getOffset();
   _data_length = shmem_msg->getDataLength();
   _data_buf_size = shmem_msg->getDataBufSize();
   _data_buf = NULL;
   if (_data_buf_size > 0)
   {
      _data_buf = new Byte[_data_buf_size];
      memcpy(_data_buf, shmem_msg->getDataBuf(), _data_buf_size);
   }
   _single_receiver = shmem_msg->getSingleReceiver();
   _reply_expected = shmem_msg->isReplyExpected();
   _requester = shmem_msg->getRequester();
   _modeled = shmem_msg->isModeled();
}

ShmemMsg*
ShmemMsg::getShmemMsg(Byte* msg_buf)
{
   ShmemMsg* shmem_msg = new ShmemMsg();
   memcpy((void*) shmem_msg, msg_buf, sizeof(*shmem_msg));
   if (shmem_msg->getDataBufSize() > 0)
   {
      shmem_msg->setDataBuf(new Byte[shmem_msg->getDataBufSize()]);
      memcpy((void*) shmem_msg->getDataBuf(), msg_buf + sizeof(*shmem_msg), shmem_msg->getDataBufSize());
   }
   return shmem_msg;
}

Byte*
ShmemMsg::makeMsgBuf()
{
   Byte* msg_buf = new Byte[getMsgLen()];
   memcpy(msg_buf, (void*) this, sizeof(*this));
   if (_data_buf_size > 0)
   {
      LOG_ASSERT_ERROR(_data_buf, "_data_buf(%p)", _data_buf);
      memcpy(msg_buf + sizeof(*this), (void*) _data_buf, _data_buf_size); 
   }
   return msg_buf; 
}

UInt32
ShmemMsg::getMsgLen()
{
   return (sizeof(*this) + _data_buf_size);
}

UInt32
ShmemMsg::getModeledLength()
{
   switch(_type)
   {
   case UNIFIED_READ_REQ:
   case UNIFIED_READ_LOCK_REQ:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits + _num_data_length_bits);

   case UNIFIED_WRITE_REQ:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits + _num_data_length_bits + _data_buf_size * 8);

   case SH_REPLY:
   case EX_REPLY:
   case ASYNC_SH_REPLY:
   case ASYNC_EX_REPLY:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits + _cache_line_size * 8);

   case UPGRADE_REPLY:
   case ASYNC_UPGRADE_REPLY:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits);

   case INV_REQ:
   case FLUSH_REQ:
   case WB_REQ:
   case INV_REPLY:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits);

   case FLUSH_REPLY:
   case WB_REPLY:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits + _cache_line_size * 8);

   case INV_FLUSH_COMBINED_REQ:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits + _num_tile_id_bits);

   case READ_REPLY:
   case READ_LOCK_REPLY:
   case REMOTE_READ_REPLY:
   case REMOTE_READ_LOCK_REPLY:
   case WRITE_UNLOCK_REQ:
   case REMOTE_WRITE_REQ:
   case REMOTE_WRITE_UNLOCK_REQ:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits + _num_data_length_bits + _data_buf_size * 8);

   case REMOTE_READ_REQ:
   case REMOTE_READ_LOCK_REQ:
   case WRITE_REPLY:
   case WRITE_UNLOCK_REPLY:
   case REMOTE_WRITE_REPLY:
   case REMOTE_WRITE_UNLOCK_REPLY:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits + _num_data_length_bits);

   case DRAM_FETCH_REQ:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits);

   case DRAM_STORE_REQ:
   case DRAM_FETCH_REPLY:
      return convertBitsToBytes(_num_msg_type_bits + _num_physical_address_bits + _cache_line_size * 8);

   default:
      LOG_PRINT_ERROR("Unrecognized Msg Type(%u)", _type);
      return 0;
   }
}

void
ShmemMsg::initializeArchitecturalParameters(UInt32 cache_line_size, UInt32 num_physical_address_bits)
{
   _num_msg_type_bits = 5;
   _num_physical_address_bits = num_physical_address_bits;
   _cache_line_size = cache_line_size;
   _num_data_length_bits = ceilLog2(_cache_line_size);
   _num_tile_id_bits = Config::getSingleton()->getTileIDLength();
}

string
ShmemMsg::getName(Type type)
{
   switch (type)
   {
   case UNIFIED_READ_REQ:
      return "UNIFIED_READ_REQ";
   case UNIFIED_READ_LOCK_REQ:
      return "UNIFIED_READ_LOCK_REQ";
   case UNIFIED_WRITE_REQ:
      return "UNIFIED_WRITE_REQ";
   case WRITE_UNLOCK_REQ:
      return "WRITE_UNLOCK_REQ";
   case SH_REPLY:
      return "SH_REPLY";
   case EX_REPLY:
      return "EX_REPLY";
   case UPGRADE_REPLY:
      return "UPGRADE_REPLY";
   case ASYNC_SH_REPLY:
      return "ASYNC_SH_REPLY";
   case ASYNC_EX_REPLY:
      return "ASYNC_EX_REPLY";
   case ASYNC_UPGRADE_REPLY:
      return "ASYNC_UPGRADE_REPLY";
   case INV_REQ:
      return "INV_REQ";
   case FLUSH_REQ:
      return "FLUSH_REQ";
   case WB_REQ:
      return "WB_REQ";
   case READ_REPLY:
      return "READ_REPLY";
   case READ_LOCK_REPLY:
      return "READ_LOCK_REPLY";
   case WRITE_REPLY:
      return "WRITE_REPLY";
   case WRITE_UNLOCK_REPLY:
      return "WRITE_UNLOCK_REPLY";
   case REMOTE_READ_REQ:
      return "REMOTE_READ_REQ";
   case REMOTE_WRITE_REQ:
      return "REMOTE_WRITE_REQ";
   case REMOTE_WRITE_UNLOCK_REQ:
      return "REMOTE_WRITE_UNLOCK_REQ";
   case REMOTE_READ_REPLY:
      return "REMOTE_READ_REPLY";
   case REMOTE_READ_LOCK_REPLY:
      return "REMOTE_READ_LOCK_REPLY";
   case REMOTE_WRITE_REPLY:
      return "REMOTE_WRITE_REPLY";
   case REMOTE_WRITE_UNLOCK_REPLY:
      return "REMOTE_WRITE_UNLOCK_REPLY";
   case DRAM_FETCH_REQ:
      return "DRAM_FETCH_REQ";
   case DRAM_STORE_REQ:
      return "DRAM_STORE_REQ";
   case DRAM_FETCH_REPLY:
      return "DRAM_FETCH_REPLY";
   case NULLIFY_REQ:
      return "NULLIFY_REQ";
   default:
      LOG_PRINT_ERROR("Unrecognized msg type(%u)", type);
      return "";
   }
}

}
