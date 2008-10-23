#ifndef __PACKET_TYPE_H__
#define __PACKET_TYPE_H__
// enums for type of network packet
enum PacketType 
{
   MIN_PACKET_TYPE = 0, 
   INVALID = MIN_PACKET_TYPE, 
   USER,
   SHARED_MEM_REQ,
   SHARED_MEM_UPDATE_EXPECTED,
   SHARED_MEM_UPDATE_UNEXPECTED,
   SHARED_MEM_ACK,
   MAX_PACKET_TYPE = SHARED_MEM_ACK
};


#endif
