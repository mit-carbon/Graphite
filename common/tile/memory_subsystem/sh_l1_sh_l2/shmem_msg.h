#pragma once

#include <cstdlib>
#include "mem_component.h"
#include "fixed_types.h"

namespace ShL1ShL2
{
   class ShmemMsg
   {
   public:
      enum Type
      {
         INVALID_MSG_TYPE = 0,
         MIN_MSG_TYPE,
         READ_REQ = MIN_MSG_TYPE,
         READ_EX_REQ,
         WRITE_REQ,
         READ_REP,
         WRITE_REP,
         MAX_MSG_TYPE = WRITE_REP,
         NUM_MSG_TYPES = MAX_MSG_TYPE - MIN_MSG_TYPE + 1
      };  
      
      ShmemMsg();
      ShmemMsg(Type msg_type,
               MemComponent::component_t sender_mem_component,
               MemComponent::component_t receiver_mem_component,
               tile_id_t requester,
               IntPtr address,
               UInt32 offset,
               UInt32 size,
               Byte* data_buf = NULL,
               UInt32 data_length = 0);
      ~ShmemMsg();

      static ShmemMsg* getShmemMsg(Byte* msg_buf);
      Byte* makeMsgBuf();
      UInt32 getMsgLen();

      ShmemMsg* clone() const;
      void release();

      // Modeling
      UInt32 getModeledLength() const;
      bool isModeled() const { return true; }

      Type getMsgType() const { return _msg_type; }
      MemComponent::component_t getSenderMemComponent() const { return _sender_mem_component; }
      MemComponent::component_t getReceiverMemComponent() const { return _receiver_mem_component; }
      tile_id_t getRequester() const { return _requester; }
      IntPtr getAddress() const { return _address; }
      UInt32 getOffset() const { return _offset; }
      UInt32 getSize() const { return _size; }
      Byte* getDataBuf() const { return _data_buf; }
      UInt32 getDataLength() const { return _data_length; }

      void setDataBuf(Byte* data_buf) { _data_buf = data_buf; }

      static void setCacheLineSize(UInt32 cache_line_size);

   private:   
      Type _msg_type;
      MemComponent::component_t _sender_mem_component;
      MemComponent::component_t _receiver_mem_component;
      tile_id_t _requester;
      IntPtr _address;
      UInt32 _offset;
      UInt32 _size;
      Byte* _data_buf;
      UInt32 _data_length;

      static UInt32 _metadata_size;
   };
}
