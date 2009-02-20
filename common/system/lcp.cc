#include "lcp.h"

#include "log.h"
#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE LCP

LCP::LCP()
{
}

LCP::~LCP()
{
}

void LCP::run()
{
   Network *net = Sim()->getCoreManager()->getCoreFromIndex(0)->getNetwork();

   net->registerCallback(SIM_THREAD_UPDATE_COMM_MAP,
                         updateCommMap,
                         net);
}

void LCP::updateCommMap(void *vp, NetPacket pkt)
{
   LOG_PRINT("CoreMap: SimThread got the UpdateCommMap message.");
   Network *net = (Network *)vp;
   UInt32 message_type;
   UInt32 core_id;
   UInt32 comm_id;
   UnstructuredBuffer recv_buff;
   
   recv_buff << make_pair(pkt.data, pkt.length);
   recv_buff >> message_type;
   recv_buff >> comm_id;
   recv_buff >> core_id;

   Sim()->getConfig()->updateCommToCoreMap(comm_id, core_id);
   bool dummy = true;
   net->netSend(pkt.sender, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}
