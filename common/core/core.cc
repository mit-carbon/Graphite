#include "core.h"

using namespace std;

int Core::coreInit(Chip *chip, int tid, int num_mod)
{
   the_chip = chip;
   core_tid = tid;
   core_num_mod = num_mod;

   network = new Network;
   network->netInit(chip, tid, num_mod);
   
   if ( g_knob_enable_performance_modeling ) 
   {
      perf_model = new PerfModel("performance modeler");
      cout << "Core[" << tid << "]: instantiated performance model" << endl;
   } else 
   {
      perf_model = (PerfModel *) NULL;    
   }   

   if ( g_knob_enable_dcache_modeling || g_knob_enable_icache_modeling ) 
   {
      ocache = new OCache("organic cache", 
                          g_knob_cache_size.Value() * k_KILO,
                          g_knob_line_size.Value(),
                          g_knob_associativity.Value(),
                          g_knob_mutation_interval.Value(),
                          g_knob_dcache_threshold_hit.Value(),
                          g_knob_dcache_threshold_miss.Value(),
                          g_knob_dcache_size.Value() * k_KILO,
                          g_knob_dcache_associativity.Value(),
                          g_knob_dcache_max_search_depth.Value(),
                          g_knob_icache_threshold_hit.Value(),
                          g_knob_icache_threshold_miss.Value(),
                          g_knob_icache_size.Value() * k_KILO,
                          g_knob_icache_associativity.Value(),
                          g_knob_icache_max_search_depth.Value());                        

      cout << "Core[" << tid << "]: instantiated organic cache model" << endl;
      cout << ocache->statsLong() << endl;
   } else 
   {
      ocache = (OCache *) NULL;
   }   

   return 0;
}

int Core::coreSendW(int sender, int receiver, char *buffer, int size)
{
   // Create a net packet
   NetPacket packet;
   packet.sender= sender;
   packet.receiver= receiver;
   packet.type = USER;
   packet.length = size;
   packet.data = new char[size];
   for(int i = 0; i < size; i++)
      packet.data[i] = buffer[i];

   network->netSend(packet);
   return 0;
}

int Core::coreRecvW(int sender, int receiver, char *buffer, int size)
{
   NetPacket packet;
   NetMatch match;

   match.sender = sender;
   match.sender_flag = true;
   match.type = USER;
   match.type_flag = true;

   packet = network->netRecv(match);

   if((unsigned)size != packet.length){
      cout << "ERROR:" << endl
           << "Received packet length is not as expected" << endl;
      exit(-1);
   }

   for(int i = 0; i < size; i++)
      buffer[i] = packet.data[i];

   return 0;
}

VOID Core::fini(int code, VOID *v, ofstream& out)
{
   if ( g_knob_enable_performance_modeling )
      perf_model->fini(code, v, out);

   if ( g_knob_enable_dcache_modeling || g_knob_enable_icache_modeling )
      ocache->fini(code,v,out);
}
