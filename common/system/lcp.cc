#include "lcp.h"
#include "simulator.h"
#include "core.h"

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
   LOG_PRINT("In LCP");

   Network *net = Sim()->getCoreManager()->getCoreFromIndex(0)->getNetwork();

   net->registerCallback(SIM_THREAD_UPDATE_COMM_MAP,
                         updateCommMap,
                         net);
}

struct CommMapUpdate
{
   UInt32 message_type;
   SInt32 core_id;
   SInt32 comm_id;
};

void LCP::updateCommMap(void *vp, NetPacket pkt)
{
   LOG_PRINT("CoreMap: SimThread got the UpdateCommMap message.");
   Network *net = (Network *)vp;
   CommMapUpdate *update;

   LOG_ASSERT_ERROR(pkt.length == sizeof(*update), "*ERROR* Packet length wrong. Expected: %d, Got: %d", sizeof(*update), pkt.length);
   update = (CommMapUpdate*)pkt.data;

   Config::getSingleton()->updateCommToCoreMap(update->comm_id, update->core_id);
   bool dummy = true;
   net->netSend(pkt.sender, MCP_RESPONSE_TYPE, (char*)&dummy, sizeof(dummy));
}
