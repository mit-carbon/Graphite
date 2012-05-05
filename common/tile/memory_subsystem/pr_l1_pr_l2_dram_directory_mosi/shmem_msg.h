#pragma once

#include "mem_component.h"
#include "fixed_types.h"
#include "common_defines.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class ShmemMsg
   {
   public:
      enum Type
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
      ShmemMsg(Type msg_type
               , MemComponent::Type sender_mem_component
               , MemComponent::Type receiver_mem_component
               , tile_id_t requester
               , tile_id_t single_receiver
               , bool reply_expected
               , IntPtr address
               , bool modeled
#ifdef TRACK_DETAILED_CACHE_COUNTERS
               , UInt64 cache_line_utilization = 0
               , UInt64 L2_cache_line_lifetime = 0
#endif
               );
      ShmemMsg(Type msg_type
               , MemComponent::Type sender_mem_component
               , MemComponent::Type receiver_mem_component
               , tile_id_t requester
               , tile_id_t single_receiver
               , bool reply_expected
               , IntPtr address
               , Byte* data_buf
               , UInt32 data_length
               , bool modeled
#ifdef TRACK_DETAILED_CACHE_COUNTERS
               , UInt64 cache_line_utilization = 0
               , UInt64 L2_cache_line_lifetime = 0
#endif
               );
      ShmemMsg(const ShmemMsg* shmem_msg);
      ~ShmemMsg();

      void clone(const ShmemMsg* shmem_msg);
      static ShmemMsg* getShmemMsg(Byte* msg_buf);
      Byte* makeMsgBuf();
      UInt32 getMsgLen();

      // Modeled Parameters
      UInt32 getModeledLength();

      Type getType() const                               { return _msg_type; }
      MemComponent::Type getSenderMemComponent() const   { return _sender_mem_component; }
      MemComponent::Type getReceiverMemComponent() const { return _receiver_mem_component; }
      tile_id_t getRequester() const                     { return _requester; }
      tile_id_t getSingleReceiver() const                { return _single_receiver; }
      bool isReplyExpected() const                       { return _reply_expected; }
      IntPtr getAddress() const                          { return _address; }
      Byte* getDataBuf() const                           { return _data_buf; }
      UInt32 getDataLength() const                       { return _data_length; }
      bool isModeled() const                             { return _modeled; }
#ifdef TRACK_DETAILED_CACHE_COUNTERS
      UInt64 getCacheLineUtilization() const             { return _cache_line_utilization; }
      UInt64 getL2CacheLineLifetime() const              { return _L2_cache_line_lifetime; }
#endif

      void setMsgType(Type msg_type) { _msg_type = msg_type; }
      void setDataBuf(Byte* data_buf) { _data_buf = data_buf; }

   private:   
      Type _msg_type;
      MemComponent::Type _sender_mem_component;
      MemComponent::Type _receiver_mem_component;
      tile_id_t _requester;
      tile_id_t _single_receiver;
      bool _reply_expected;
      IntPtr _address;
      Byte* _data_buf;
      UInt32 _data_length;
      bool _modeled;
#ifdef TRACK_DETAILED_CACHE_COUNTERS
      UInt64 _cache_line_utilization;
      UInt64 _L2_cache_line_lifetime;
#endif
   };
}
