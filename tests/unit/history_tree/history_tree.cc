#include "carbon_user.h"
#include "fixed_types.h"
#include "queue_model_history_tree.h"
#include "queue_model_history_list.h"

#define NUM_PACKETS  10

UInt64 pkts[NUM_PACKETS][2] = {
   {10, 10},
   {21, 10},
   {32, 10},
   {43, 10},
   {0, 1},
   {0, 10},
   {45, 10},
   {60, 4},
   {70, 8},
   {75, 10}
};

int main(int argc, char* argv[])
{
   CarbonStartSim(argc, argv);

   QueueModelHistoryTree queue_model(1);
   
   for (SInt32 i = 0; i < NUM_PACKETS; i++)
   {
      printf("\n\nQueue Delay: Pkt(%llu,%llu)\n", \
            (long long unsigned int) pkts[i][0], \
            (long long unsigned int) pkts[i][1]);
      
      UInt64 queue_delay = queue_model.computeQueueDelay(pkts[i][0], pkts[i][1]);

      printf("Queue Delay: Pkt(%llu,%llu) -> Queue Delay(%llu)\n\n", \
            (long long unsigned int) pkts[i][0], \
            (long long unsigned int) pkts[i][1], \
            (long long unsigned int) queue_delay);
   }
   
   CarbonStopSim();
   
   return 0;
}
