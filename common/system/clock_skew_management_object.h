#pragma once

#include <string>

// Forward Decls
class Core;
class Network;
class UnstructuredBuffer;
class NetPacket;

#include "fixed_types.h"
#include "time_types.h"

class ClockSkewManagementObject
{
public:
   enum Scheme
   {
      LAX = 0,
      LAX_BARRIER,
      LAX_P2P,
      NUM_SCHEMES
   };

   static Scheme parseScheme(std::string scheme);
};
 
void ClockSkewManagementClientNetworkCallback(void* obj, NetPacket packet);

class ClockSkewManagementClient : public ClockSkewManagementObject
{
protected:
   ClockSkewManagementClient() {}

public:
   ~ClockSkewManagementClient() {}
   static ClockSkewManagementClient* create(std::string scheme_str, Core* core);

   virtual void enable() = 0;
   virtual void disable() = 0;
   virtual void synchronize(Time curr_time = Time(0)) = 0;
   virtual void netProcessSyncMsg(const NetPacket& recv_pkt) = 0;
};

class ClockSkewManagementManager : public ClockSkewManagementObject
{
protected:
   ClockSkewManagementManager() {}

public:
   ~ClockSkewManagementManager() {}
   static ClockSkewManagementManager* create(std::string scheme_str);

   virtual void processSyncMsg(Byte* msg) = 0;
};

class ClockSkewManagementServer : public ClockSkewManagementObject
{
protected:
   ClockSkewManagementServer() {}

public:
   ~ClockSkewManagementServer() {}
   static ClockSkewManagementServer* create(std::string scheme_str, Network& network, UnstructuredBuffer& recv_buff);

   virtual void processSyncMsg(core_id_t core_id) = 0;
   virtual void signal() = 0;
};
