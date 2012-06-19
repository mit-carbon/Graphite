#pragma once

#include <string>
using std::string;

#include "mem_component.h"
#include "fixed_types.h"

namespace HybridProtocol_PPMOSI_SS
{

class ShmemMsg
{
public:
   enum Type
   {
      INVALID,
     
      // Requests sent from the L2 cache to the directory
      // When the requests are sent, it is not known whether
      // these cache lines are in the Remote mode or Private mode 
      UNIFIED_READ_REQ,
      UNIFIED_READ_LOCK_REQ,
      UNIFIED_WRITE_REQ,
      WRITE_UNLOCK_REQ,
     
      // These replies sent from the directory to the L2 cache
      // in response to a Private sharer
      SH_REPLY,
      EX_REPLY,
      UPGRADE_REPLY,

      // Asynchronous replies for processing remote requests
      ASYNC_SH_REPLY,
      ASYNC_EX_REPLY,
      ASYNC_UPGRADE_REPLY,

      // These requests/replies sent from the directory-to-L2/L2-to-directory
      // are exclusive to the PrL1PrL2MOSI protocol
      // The RemoteAccess protocol, since it keeps cache lines exclusively
      // at the home node does not have these requests
      INV_REQ,
      FLUSH_REQ,
      WB_REQ,
      INV_FLUSH_COMBINED_REQ,
      INV_REPLY,
      FLUSH_REPLY,
      WB_REPLY,

      // These replies sent from the home node (L2 cache) to the requester
      // are exclusive to the Remote-Access / Remote-Store protocols
      READ_REPLY,
      READ_LOCK_REPLY,
      WRITE_REPLY,
      WRITE_UNLOCK_REPLY,

      // Read/Write data from a remote node
      REMOTE_READ_REQ,
      REMOTE_READ_LOCK_REQ,
      REMOTE_WRITE_REQ,
      REMOTE_WRITE_UNLOCK_REQ,
      REMOTE_READ_REPLY,
      REMOTE_READ_LOCK_REPLY,
      REMOTE_WRITE_REPLY,
      REMOTE_WRITE_UNLOCK_REPLY,

      // Read/write cache-line from/to a memory-controller
      DRAM_FETCH_REQ,
      DRAM_STORE_REQ,
      DRAM_FETCH_REPLY,

      // This request is used to replace an entry in the directory
      // due to capacity/conflict misses
      NULLIFY_REQ
   }; 

   ShmemMsg();

   ShmemMsg(Type type,
            MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
            IntPtr address, 
            tile_id_t requester, bool modeled);
   ShmemMsg(Type type,
            MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
            IntPtr address,
            Byte* data_buf, UInt32 data_buf_size,
            tile_id_t requester, bool modeled);

   ShmemMsg(Type type,
            MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
            IntPtr address,
            UInt32 offset, UInt32 data_length,
            tile_id_t requester, bool modeled);
   ShmemMsg(Type type,
            MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
            IntPtr address,
            UInt32 offset, UInt32 data_length,
            Byte* data_buf, UInt32 data_buf_size,
            tile_id_t requester, bool modeled);

   ShmemMsg(Type type,
            MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
            IntPtr address,
            tile_id_t single_receiver, bool reply_expected,
            tile_id_t requester, bool modeled);
   ShmemMsg(Type type,
            MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component,
            IntPtr address,
            Byte* data_buf, UInt32 data_buf_size,
            tile_id_t single_receiver, bool reply_expected,
            tile_id_t requester, bool modeled);
   
   ShmemMsg(const ShmemMsg* shmem_msg);

   ~ShmemMsg();

   void assign(const ShmemMsg* shmem_msg);

   // Message packing utilities
   static ShmemMsg* getShmemMsg(Byte* msg_buf);
   Byte* makeMsgBuf();
   UInt32 getMsgLen();

   // Get the msg type as a string
   static string getName(Type type);

   // Non-static member functions
   Type getType() const { return _type; }
   MemComponent::Type getSenderMemComponent() const { return _sender_mem_component; }
   MemComponent::Type getReceiverMemComponent() const { return _receiver_mem_component; }
   IntPtr getAddress() const { return _address; }
   UInt32 getOffset() const { return _offset; }
   UInt32 getDataLength() const { return _data_length; }
   Byte* getDataBuf() const { return _data_buf; }
   UInt32 getDataBufSize() const { return _data_buf_size; }
   tile_id_t getSingleReceiver() const { return _single_receiver; }
   bool isReplyExpected() const { return _reply_expected; }
   tile_id_t getRequester() const { return _requester; }
   bool isModeled() const { return _modeled; }
   UInt32 getUtilization() const  { return _utilization; }

   void setType(Type type) { _type = type; }
   void setDataBuf(Byte* data_buf) { _data_buf = data_buf; }

   UInt32 getModeledLength();

   // Static member functions
   static void initializeArchitecturalParameters(UInt32 cache_line_size, UInt32 num_physical_address_bits);

private:
   // Non-static data fields
   Type _type;
   MemComponent::Type _sender_mem_component;
   MemComponent::Type _receiver_mem_component;
   IntPtr _address;
   UInt32 _offset;
   UInt32 _data_length;
   Byte* _data_buf;
   UInt32 _data_buf_size;
   tile_id_t _single_receiver;
   bool _reply_expected;
   tile_id_t _requester;
   bool _modeled;
   UInt32 _utilization;

   // Static data fields
   static UInt32 _num_msg_type_bits;
   static UInt32 _num_physical_address_bits;
   static UInt32 _cache_line_size;
   static UInt32 _num_data_length_bits;
   static UInt32 _num_tile_id_bits;
   
   // Initialize the MSG with default values
   void initialize();
};

}
