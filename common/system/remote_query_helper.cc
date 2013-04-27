#include <cassert>
#include <utility>
using std::make_pair;
#include "remote_query_helper.h"
#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "packetize.h"
#include "packet_type.h"

void
RemoteQueryHelperNetworkCallback(void* obj, NetPacket packet)
{
   RemoteQueryHelper* helper = (RemoteQueryHelper*) obj;
   assert(helper);
   helper->handleQuery(packet);
}

RemoteQueryHelper::RemoteQueryHelper(Tile* tile)
   : _tile(tile)
{
   _tile->getNetwork()->registerCallback(REMOTE_QUERY, RemoteQueryHelperNetworkCallback, this);
}

RemoteQueryHelper::~RemoteQueryHelper()
{
   _tile->getNetwork()->unregisterCallback(REMOTE_QUERY);
}

void
RemoteQueryHelper::handleQuery(const NetPacket& packet)
{
   UnstructuredBuffer recv_buf;
   recv_buf << make_pair(packet.data, packet.length);
   
   RemoteQueryType query_type;
   recv_buf >> query_type;
   switch (query_type)
   {
   case CORE_TIME:
      __getCoreTime(packet.sender.tile_id);
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized remote query type(%u)", query_type);
      break;
   }
}

Time
RemoteQueryHelper::getCoreTime(tile_id_t tile_id)
{
   RemoteQueryType query_type = CORE_TIME;
   Network* net = _tile->getNetwork();
   net->netSend(Tile::getMainCoreId(tile_id), REMOTE_QUERY, &query_type, sizeof(query_type));
   
   NetPacket reply = net->netRecv(Tile::getMainCoreId(tile_id), Tile::getMainCoreId(_tile->getId()), REMOTE_QUERY_RESPONSE);
   Time time = *((Time*) reply.data);
   delete [] (Byte*) reply.data;
   return time;
}

void
RemoteQueryHelper::__getCoreTime(tile_id_t sender)
{
   Time curr_time = _tile->getCore()->getModel()->getCurrTime();
   Network* net = _tile->getNetwork();
   net->netSend(Tile::getMainCoreId(sender), REMOTE_QUERY_RESPONSE, &curr_time, sizeof(curr_time));
}
