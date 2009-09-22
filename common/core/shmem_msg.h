#ifndef __SHMEM_MSG_H__
#define __SHMEM_MSG_H__

#include "mem_component.h"
#include "fixed_types.h"

class ShmemMsg
{
   public:
      enum msg_t
      {
         INVALID_MSG_TYPE = 0,
         MIN_MSG_TYPE,
         EX_REQ = MIN_MSG_TYPE,
         SH_REQ,
         INV_REQ,
         FLUSH_REQ,
         WB_REQ,
         EX_REP,
         SH_REP,
         INV_REP,
         FLUSH_REP,
         WB_REP,
         GET_DATA_REQ,
         PUT_DATA_REQ,
         GET_DATA_REP,
         MAX_MSG_TYPE = GET_DATA_REP,
         NUM_MSG_TYPES = MAX_MSG_TYPE - MIN_MSG_TYPE + 1
      };   
   
      msg_t m_msg_type;
      MemComponent::component_t m_sender_mem_component;
      MemComponent::component_t m_receiver_mem_component;
      IntPtr m_address;
      Byte* m_data_buf;
      UInt32 m_data_length;

      ShmemMsg();
      ShmemMsg(msg_t msg_type,
            MemComponent::component_t sender_mem_component,
            MemComponent::component_t receiver_mem_component,
            IntPtr address,
            Byte* data_buf,
            UInt32 data_length);
      ShmemMsg(ShmemMsg* shmem_msg);

      ~ShmemMsg();

      static ShmemMsg* getShmemMsg(Byte* msg_buf);
      Byte* makeMsgBuf();
      UInt32 getMsgLen();
 
};

#endif /* __SHMEM_MSG_H__ */
