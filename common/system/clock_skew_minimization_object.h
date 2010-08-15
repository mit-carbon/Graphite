#ifndef __CLOCK_SKEW_MINIMIZATION_OBJECT_H__
#define __CLOCK_SKEW_MINIMIZATION_OBJECT_H__

#include <string>

// Forward Decls
class Core;
class Network;
class UnstructuredBuffer;
class NetPacket;

#include "fixed_types.h"

class ClockSkewMinimizationObject
{
   public:
      enum Scheme
      {
         NONE = 0,
         BARRIER,
         RING,
         RANDOM_PAIRS,
         NUM_SCHEMES
      };

      static Scheme parseScheme(std::string scheme);
};
 
void ClockSkewMinimizationClientNetworkCallback(void* obj, NetPacket packet);

class ClockSkewMinimizationClient : public ClockSkewMinimizationObject
{
protected:
   ClockSkewMinimizationClient() {}

public:
   ~ClockSkewMinimizationClient() {}
   static ClockSkewMinimizationClient* create(std::string scheme_str, Core* core);

   virtual void enable() = 0;
   virtual void disable() = 0;
   virtual void reset() = 0;
   virtual void synchronize(UInt64 cycle_count = 0) = 0;
   virtual void netProcessSyncMsg(const NetPacket& recv_pkt) = 0;
};

class ClockSkewMinimizationManager : public ClockSkewMinimizationObject
{
protected:
   ClockSkewMinimizationManager() {}

public:
   ~ClockSkewMinimizationManager() {}
   static ClockSkewMinimizationManager* create(std::string scheme_str);

   virtual void processSyncMsg(Byte* msg) = 0;
};

class ClockSkewMinimizationServer : public ClockSkewMinimizationObject
{
protected:
   ClockSkewMinimizationServer() {}

public:
   ~ClockSkewMinimizationServer() {}
   static ClockSkewMinimizationServer* create(std::string scheme_str, Network& network, UnstructuredBuffer& recv_buff);

   virtual void processSyncMsg(core_id_t core_id) = 0;
   virtual void signal() = 0;
};

#endif /* __CLOCK_SKEW_MINIMIZATION_OBJECT_H__ */
