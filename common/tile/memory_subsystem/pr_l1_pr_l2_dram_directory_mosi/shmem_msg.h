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

      ShmemMsg();
      ShmemMsg(msg_t msg_type,
            MemComponent::component_t sender_mem_component,
            MemComponent::component_t receiver_mem_component,
            tile_id_t requester,
            tile_id_t single_receiver,
            bool reply_expected,
            IntPtr address,
            bool modeled);
      ShmemMsg(msg_t msg_type,
            MemComponent::component_t sender_mem_component,
            MemComponent::component_t receiver_mem_component,
            tile_id_t requester,
            tile_id_t single_receiver,
            bool reply_expected,
            IntPtr address,
            Byte* data_buf,
            UInt32 data_length,
            bool modeled);
      ShmemMsg(const ShmemMsg* shmem_msg);

      ~ShmemMsg();

      void clone(const ShmemMsg* shmem_msg);
      static ShmemMsg* getShmemMsg(Byte* msg_buf);
      Byte* makeMsgBuf();
      UInt32 getMsgLen();

      // Modeled Parameters
      UInt32 getModeledLength();

      msg_t getMsgType() const { return _msg_type; }
      MemComponent::component_t getSenderMemComponent() const { return _sender_mem_component; }
      MemComponent::component_t getReceiverMemComponent() const { return _receiver_mem_component; }
      tile_id_t getRequester() const { return _requester; }
      tile_id_t getSingleReceiver() const { return _single_receiver; }
      bool isReplyExpected() const { return _reply_expected; }
      IntPtr getAddress() const { return _address; }
      Byte* getDataBuf() const { return _data_buf; }
      UInt32 getDataLength() const { return _data_length; }
      bool isModeled() const { return _modeled; }

      void setMsgType(msg_t msg_type) { _msg_type = msg_type; }
      void setDataBuf(Byte* data_buf) { _data_buf = data_buf; }

   private:   
      msg_t _msg_type;
      MemComponent::component_t _sender_mem_component;
      MemComponent::component_t _receiver_mem_component;
      tile_id_t _requester;
      tile_id_t _single_receiver;
      bool _reply_expected;
      IntPtr _address;
      Byte* _data_buf;
      UInt32 _data_length;
      bool _modeled;
   };
}
