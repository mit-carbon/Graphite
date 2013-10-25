#include "core.h"
#include "network.h"
#include "transport.h"
#include "packetize.h"

#include "clock_skew_management_object.h"
#include "lax_barrier_sync_client.h"
#include "lax_barrier_sync_server.h"
#include "lax_p2p_sync_client.h"

#include "log.h"

void ClockSkewManagementClientNetworkCallback(void* obj, NetPacket packet)
{
   ClockSkewManagementClient *client = (ClockSkewManagementClient*) obj;
   assert(client != NULL);

   client->netProcessSyncMsg(packet);
}

ClockSkewManagementObject::Scheme
ClockSkewManagementObject::parseScheme(std::string scheme)
{
   if (scheme == "lax")
      return LAX;
   else if (scheme == "lax_barrier")
      return LAX_BARRIER;
   else if (scheme == "lax_p2p")
      return LAX_P2P;
   else
   {
      LOG_PRINT_ERROR("Unrecognized clock skew management scheme: %s", scheme.c_str());
      return NUM_SCHEMES;
   }
}

ClockSkewManagementClient*
ClockSkewManagementClient::create(std::string scheme_str, Core* core)
{
   Scheme scheme = ClockSkewManagementObject::parseScheme(scheme_str);

   switch (scheme)
   {
      case LAX:
         return (ClockSkewManagementClient*) NULL;

      case LAX_BARRIER:
         return new LaxBarrierSyncClient(core);

      case LAX_P2P:
         return new LaxP2PSyncClient(core);

      default:
         LOG_PRINT_ERROR("Unrecognized scheme: %u", scheme);
         return (ClockSkewManagementClient*) NULL;
   }
}

ClockSkewManagementManager*
ClockSkewManagementManager::create(std::string scheme_str)
{
   Scheme scheme = ClockSkewManagementObject::parseScheme(scheme_str);

   switch (scheme)
   {
      case LAX:
      case LAX_BARRIER:
      case LAX_P2P:
         return (ClockSkewManagementManager*) NULL;

      default:
         LOG_PRINT_ERROR("Unrecognized scheme: %u", scheme);
         return (ClockSkewManagementManager*) NULL;
   }
}

ClockSkewManagementServer*
ClockSkewManagementServer::create(std::string scheme_str, Network& network, UnstructuredBuffer& recv_buff)
{
   Scheme scheme = ClockSkewManagementObject::parseScheme(scheme_str);

   switch (scheme)
   {
      case LAX:
         return (ClockSkewManagementServer*) NULL;

      case LAX_BARRIER:
         return new LaxBarrierSyncServer(network, recv_buff);

      case LAX_P2P:
         return (ClockSkewManagementServer*) NULL;

      default:
         LOG_PRINT_ERROR("Unrecognized scheme: %u", scheme);
         return (ClockSkewManagementServer*) NULL;
   }
}
