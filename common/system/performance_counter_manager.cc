#include "performance_counter_manager.h"
#include "simulator.h"
#include "network.h"
#include "transport.h"
#include "log.h"

PerformanceCounterManager::PerformanceCounterManager()
{}

PerformanceCounterManager::~PerformanceCounterManager()
{}

void
PerformanceCounterManager::togglePerformanceCounters(Byte* msg)
{
   SInt32 msg_type = *((SInt32*) msg);
   switch (msg_type)
   {
   case ENABLE:
      {
         Simulator::enablePerformanceModelsInCurrentProcess();
         // Send ACK back to tile 0
         NetPacket ack(Time(0) /* time */, LCP_TOGGLE_PERFORMANCE_COUNTERS_ACK /* packet type */,
                       0 /* sender - doesn't matter */, 0 /* receiver */,
                       0 /* length */, NULL /* data */);
         Byte* buffer = ack.makeBuffer();
         Transport::Node* transport = Transport::getSingleton()->getGlobalNode();
         transport->send(0, buffer, ack.bufferSize());
      }
      break;
   
   case DISABLE:
      {
         Simulator::disablePerformanceModelsInCurrentProcess();
         // Send ACK back to tile 0
         NetPacket ack(Time(0) /* time */, LCP_TOGGLE_PERFORMANCE_COUNTERS_ACK /* packet type */,
                       0 /* sender - doesn't matter */, 0 /* receiver */,
                       0 /* length */, NULL /* data */);
         Byte* buffer = ack.makeBuffer();
         Transport::Node* transport = Transport::getSingleton()->getGlobalNode();
         transport->send(0, buffer, ack.bufferSize());
      }
      break;
   
   default:
      LOG_PRINT_ERROR("Unrecognized msg type(%i)", msg_type);
      break;
   }
}
