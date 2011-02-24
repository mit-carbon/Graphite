#pragma once

#include "mem_component.h"
#include "fixed_types.h"

namespace PrL1PrL2DramDirectoryMOSI
{
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
            INV_FLUSH_COMBINED_REQ,
            EX_REP,
            SH_REP,
            UPGRADE_REP,
            INV_REP,
            FLUSH_REP,
            WB_REP,
            NULLIFY_REQ,
            MAX_MSG_TYPE = NULLIFY_REQ,
            NUM_MSG_TYPES = MAX_MSG_TYPE - MIN_MSG_TYPE + 1
         }; 

      private:   
         msg_t m_msg_type;
         MemComponent::component_t m_sender_mem_component;
         MemComponent::component_t m_receiver_mem_component;
         tile_id_t m_requester;
         tile_id_t m_single_receiver;
         bool m_reply_expected;
         IntPtr m_address;
         Byte* m_data_buf;
         UInt32 m_data_length;

      public:
         ShmemMsg();
         ShmemMsg(msg_t msg_type,
               MemComponent::component_t sender_mem_component,
               MemComponent::component_t receiver_mem_component,
               tile_id_t requester,
               tile_id_t single_receiver,
               bool reply_expected,
               IntPtr address,
               Byte* data_buf = 0,
               UInt32 data_length = 0);
         ShmemMsg(ShmemMsg* shmem_msg);

         ~ShmemMsg();

         void clone(ShmemMsg* shmem_msg);
         static ShmemMsg* getShmemMsg(Byte* msg_buf);
         Byte* makeMsgBuf();
         UInt32 getMsgLen();

         // Modeled Parameters
         UInt32 getModeledLength();
         
         msg_t getMsgType() { return m_msg_type; }
         MemComponent::component_t getSenderMemComponent() { return m_sender_mem_component; }
         MemComponent::component_t getReceiverMemComponent() { return m_receiver_mem_component; }
         tile_id_t getRequester() { return m_requester; }
         tile_id_t getSingleReceiver() { return m_single_receiver; }
         bool isReplyExpected() { return m_reply_expected; }
         IntPtr getAddress() { return m_address; }
         Byte* getDataBuf() { return m_data_buf; }
         UInt32 getDataLength() { return m_data_length; }

         void setMsgType(msg_t msg_type) { m_msg_type = msg_type; }
         void setSenderMemComponent(MemComponent::component_t sender_mem_component) { m_sender_mem_component = sender_mem_component; }
         void setAddress(IntPtr address) { m_address = address; }
         void setDataBuf(Byte* data_buf) { m_data_buf = data_buf; }
    
   };
}
