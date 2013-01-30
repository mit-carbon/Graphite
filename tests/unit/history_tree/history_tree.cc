#include <cstdlib>
#include "carbon_user.h"
#include "fixed_types.h"
#include "queue_model_history_tree.h"
#include "queue_model_history_list.h"

#define NUM_PACKETS  10

UInt64 pkt_cfg[NUM_PACKETS][3] = {
   {10, 10, 0},
   {21, 10, 0},
   {32, 10, 0},
   {43, 10, 0},
   {0,  1,  0},
   {0,  10, 53},
   {45, 10, 18},
   {60, 4,  13},
   {70, 8,  7},
   {75, 10, 10}
};

int main(int argc, char* argv[])
{
   CarbonStartSim(argc, argv);
   printf("Starting History-Tree test\n");

   QueueModelHistoryTree queue_model(1);
   
   for (SInt32 i = 0; i < NUM_PACKETS; i++)
   {
      UInt64 queue_delay = queue_model.computeQueueDelay(pkt_cfg[i][0], pkt_cfg[i][1]);
      if (queue_delay != pkt_cfg[i][2])
      {
         fprintf(stderr, "*ERROR* Queue Delay: Pkt(%llu,%llu), Expected(%llu), Got(%llu)\n", 
                 (long long unsigned int) pkt_cfg[i][0],
                 (long long unsigned int) pkt_cfg[i][1],
                 (long long unsigned int) pkt_cfg[i][2],
                 (long long unsigned int) queue_delay);
         fprintf(stderr, "History-Tree test: FAILED\n");
         exit(EXIT_FAILURE);
      }

      printf("Queue Delay: Pkt(%llu,%llu), Delay(%llu)\n",
            (long long unsigned int) pkt_cfg[i][0],
            (long long unsigned int) pkt_cfg[i][1],
            (long long unsigned int) queue_delay);
   }
   
   printf("History-Tree test: SUCCESS\n");
   CarbonStopSim();
   
   return 0;
}
