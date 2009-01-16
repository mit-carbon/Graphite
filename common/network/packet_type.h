#ifndef __PACKET_TYPE_H__
#define __PACKET_TYPE_H__

enum PacketType 
{
   INVALID,
   USER,
   SHARED_MEM_REQ,
   SHARED_MEM_EVICT,
   SHARED_MEM_RESPONSE,
   SHARED_MEM_UPDATE_UNEXPECTED,
   SHARED_MEM_ACK,
   SHARED_MEM_TERMINATE_THREADS,
   MCP_REQUEST_TYPE,
   MCP_RESPONSE_TYPE,
   MCP_UTILIZATION_UPDATE_TYPE,
   NUM_PACKET_TYPES
};

// This defines the different static network types
enum EStaticNetwork
   {
      STATIC_NETWORK_USER,
      STATIC_NETWORK_MEMORY,
      STATIC_NETWORK_SYSTEM,
      NUM_STATIC_NETWORKS
   };

// Packets are routed to a static network based on their type. This
// gives the static network to use for a given packet type.
static EStaticNetwork g_type_to_static_network_map[] __attribute__((unused)) = 
   {
      STATIC_NETWORK_SYSTEM, // INVALID
      STATIC_NETWORK_USER,   // USER
      STATIC_NETWORK_MEMORY, // SM_REQ
      STATIC_NETWORK_MEMORY, // SM_EVICT
      STATIC_NETWORK_MEMORY, // SM_RESPONSE
      STATIC_NETWORK_MEMORY, // SM_UPDATE_UNEXPECTED
      STATIC_NETWORK_MEMORY, // SM_ACK
      STATIC_NETWORK_MEMORY, // SM_TERMINATE_THREADS
      STATIC_NETWORK_SYSTEM, // MCP_REQ
      STATIC_NETWORK_SYSTEM, // MCP_RESP
      STATIC_NETWORK_SYSTEM, // MCP_UTIL
   };

#endif
