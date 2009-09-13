#include <string.h>
#include "shmem_msg.h"

ShmemMsg::ShmemMsg() :
   m_msg_type(INVALID_MSG_TYPE),
   m_sender_mem_component(MemComponent::INVALID_MEM_COMPONENT),
   m_receiver_mem_component(MemComponent::INVALID_MEM_COMPONENT),
   m_address(INVALID_ADDRESS),
   m_data_buf(NULL),
   m_data_length(0)
{}

ShmemMsg::ShmemMsg(msg_t msg_type,
      MemComponent::component_t sender_mem_component,
      MemComponent::component_t receiver_mem_component,
      IntPtr address,
      Byte* data_buf,
      UInt32 data_length) :
   m_msg_type(msg_type),
   m_sender_mem_component(sender_mem_component),
   m_receiver_mem_component(receiver_mem_component),
   m_address(address),
   m_data_buf(data_buf),
   m_data_length(data_length)
{}

ShmemMsg::ShmemMsg(ShmemMsg* shmem_msg) :
   m_msg_type(shmem_msg->m_msg_type),
   m_sender_mem_component(shmem_msg->m_sender_mem_component),
   m_receiver_mem_component(shmem_msg->m_receiver_mem_component),
   m_address(shmem_msg->m_address),
   m_data_buf(shmem_msg->m_data_buf),
   m_data_length(shmem_msg->m_data_length)
{}


ShmemMsg::~ShmemMsg()
{}

ShmemMsg*
ShmemMsg::getShmemMsg(Byte* msg_buf)
{
   ShmemMsg* shmem_msg = new ShmemMsg();
   memcpy((void*) shmem_msg, msg_buf, sizeof(*shmem_msg));
   if (shmem_msg->m_data_length > 0)
   {
      shmem_msg->m_data_buf = new Byte[shmem_msg->m_data_length];
      memcpy((void*) shmem_msg->m_data_buf, msg_buf + sizeof(*shmem_msg), shmem_msg->m_data_length);
   }
   return shmem_msg;
}

Byte*
ShmemMsg::makeMsgBuf()
{
   Byte* msg_buf = new Byte[getMsgLen()];
   memcpy(msg_buf, (void*) this, sizeof(*this));
   if (m_data_length > 0)
      memcpy(msg_buf + sizeof(*this), (void*) m_data_buf, m_data_length); 

   return msg_buf; 
}

UInt32
ShmemMsg::getMsgLen()
{
   return (sizeof(*this) + m_data_length);
}
