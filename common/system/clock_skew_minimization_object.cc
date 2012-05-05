#include "core.h"
#include "network.h"
#include "transport.h"
#include "packetize.h"

#include "clock_skew_minimization_object.h"
#include "lax_barrier_sync_client.h"
#include "lax_barrier_sync_server.h"
#include "lax_p2p_sync_client.h"

#include "log.h"

void ClockSkewMinimizationClientNetworkCallback(void* obj, NetPacket packet)
{
   ClockSkewMinimizationClient *client = (ClockSkewMinimizationClient*) obj;
   assert(client != NULL);

   client->netProcessSyncMsg(packet);
}

ClockSkewMinimizationObject::Scheme
ClockSkewMinimizationObject::parseScheme(std::string scheme)
{
   if (scheme == "lax")
      return LAX;
   else if (scheme == "lax_barrier")
      return LAX_BARRIER;
   else if (scheme == "lax_p2p")
      return LAX_P2P;
   else
   {
      LOG_PRINT_ERROR("Unrecognized clock skew minimization scheme: %s", scheme.c_str());
      return NUM_SCHEMES;
   }
}

ClockSkewMinimizationClient*
ClockSkewMinimizationClient::create(std::string scheme_str, Core* core)
{
   Scheme scheme = ClockSkewMinimizationObject::parseScheme(scheme_str);

   switch (scheme)
   {
      case LAX:
         return (ClockSkewMinimizationClient*) NULL;

      case LAX_BARRIER:
         return new LaxBarrierSyncClient(core);

      case LAX_P2P:
         return new LaxP2PSyncClient(core);

      default:
         LOG_PRINT_ERROR("Unrecognized scheme: %u", scheme);
         return (ClockSkewMinimizationClient*) NULL;
   }
}

ClockSkewMinimizationManager*
ClockSkewMinimizationManager::create(std::string scheme_str)
{
   Scheme scheme = ClockSkewMinimizationObject::parseScheme(scheme_str);

   switch (scheme)
   {
      case LAX:
      case LAX_BARRIER:
      case LAX_P2P:
         return (ClockSkewMinimizationManager*) NULL;

      default:
         LOG_PRINT_ERROR("Unrecognized scheme: %u", scheme);
         return (ClockSkewMinimizationManager*) NULL;
   }
}

ClockSkewMinimizationServer*
ClockSkewMinimizationServer::create(std::string scheme_str, Network& network, UnstructuredBuffer& recv_buff)
{
   Scheme scheme = ClockSkewMinimizationObject::parseScheme(scheme_str);

   switch (scheme)
   {
      case LAX:
         return (ClockSkewMinimizationServer*) NULL;

      case LAX_BARRIER:
         return new LaxBarrierSyncServer(network, recv_buff);

      case LAX_P2P:
         return (ClockSkewMinimizationServer*) NULL;

      default:
         LOG_PRINT_ERROR("Unrecognized scheme: %u", scheme);
         return (ClockSkewMinimizationServer*) NULL;
   }
}
