#include "carbon_user.h"
#include "fixed_types.h"
#include "queue_model_history_tree.h"

#define NUM_PACKETS  43

/*
UInt64 pkts[NUM_PACKETS][2] = { {468, 13}, \
                                {523, 13}, \
                                {1107, 13}, \
                                {1659, 13}, \
                                {1039, 13} };
 */
 
UInt64 pkts[NUM_PACKETS][2] = {
{36754, 13}, 
{48383, 13}, 
{65226, 13}, 
{65914, 13}, 
{67242, 13}, 
{67890, 13}, 
{67974, 13}, 
{67974, 13}, 
{71103, 13}, 
{87499, 13}, 
{93073, 13}, 
{94286, 13}, 
{100358, 13}, 
{100601, 13}, 
{101599, 13}, 
{101392, 13}, 
{102655, 13}, 
{101943, 13}, 
{105829, 13}, 
{109700, 13}, 
{112179, 13}, 
{112909, 13}, 
{101769, 13}, 
{101799, 13}, 
{101919, 13}, 
{101459, 13}, 
{101690, 13}, 
{100901, 13}, 
{101653, 13}, 
{101888, 13}, 
{100373, 13}, 
{100815, 13}, 
{101599, 13}, 
{101851, 13}, 
{101737, 13}, 
{101980, 13}, 
{101826, 13}, 
{102073, 13}, 
{144678, 13}, 
{101747, 13}, 
{101965, 13}, 
{101776, 13}, 
{102305, 13}
};

int main(int argc, char* argv[])
{
   CarbonStartSim(argc, argv);

   QueueModelHistoryTree queue_model(13);
   
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
