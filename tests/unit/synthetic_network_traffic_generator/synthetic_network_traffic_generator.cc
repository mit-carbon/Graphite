#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cmath>
#include "simulator.h"
#include "core_manager.h"
#include "config.h"
#include "core.h"
#include "network.h"
#include "network_model.h"
#include "clock_skew_minimization_object.h"
#include "carbon_user.h"
#include "utils.h"

class RandNum
{
   public:
      RandNum(double start, double end):
         _start(start), _end(end) 
      { 
         srand48_r(time(NULL), &rand_buffer); 
      }
      ~RandNum() {}
      double next()
      {
         double result;
         drand48_r(&rand_buffer, &result);
         return (result * (_end - _start) + _start);
      }

   private:
      struct drand48_data rand_buffer;
      double _start;
      double _end;
};

enum NetworkTrafficType
{
   UNIFORM_RANDOM = 0,
   BIT_COMPLEMENT,
   SHUFFLE,
   TRANSPOSE,
   TORNADO,
   NEAREST_NEIGHBOR,
   NUM_NETWORK_TRAFFIC_TYPES
};

void* sendNetworkTraffic(void*);
void uniformRandomTrafficGenerator(int core_id, vector<int>& send_vec, vector<int>& receive_vec);
void bitComplementTrafficGenerator(int core_id, vector<int>& send_vec, vector<int>& receive_vec);
void shuffleTrafficGenerator(int core_id, vector<int>& send_vec, vector<int>& receive_vec);
void transposeTrafficGenerator(int core_id, vector<int>& send_vec, vector<int>& receive_vec);
void tornadoTrafficGenerator(int core_id, vector<int>& send_vec, vector<int>& receive_vec);
void nearestNeighborTrafficGenerator(int core_id, vector<int>& send_vec, vector<int>& receive_vec);

bool canSendPacket(double offered_load, RandNum& rand_num);
void synchronize(UInt64 time, Core* core);
void printHelpMessage();

string _traffic_pattern_type = "uniform_random";      // Network Traffic Pattern Type
double _offered_load = 0.1;                           // Number of packets injected per core per cycle
SInt32 _packet_size = 8;                              // Size of each Packet in Bytes
UInt64 _total_packets = 10000;                        // Total number of packets injected into the network per core

PacketType _packet_type = USER_2;                     // Type of each packet (so as to send on 2nd user network)
carbon_barrier_t _global_barrier;
SInt32 _num_cores;

int main(int argc, char* argv[])
{
   CarbonStartSim(argc, argv);

   Simulator::enablePerformanceModelsInCurrentProcess();

   // Read Command Line Arguments
   for (SInt32 i = 1; i < argc-1; i += 2)
   {
      if (string(argv[i]) == "-p")
         _traffic_pattern_type = string(argv[i+1]);
      else if (string(argv[i]) == "-l")
         _offered_load = (double) atof(argv[i+1]);
      else if (string(argv[i]) == "-s")
         _packet_size = (SInt32) atoi(argv[i+1]);
      else if (string(argv[i]) == "-N")
         _total_packets = (UInt64) atoi(argv[i+1]);
      else if (string(argv[i]) == "-c") // Simulator arguments
         break;
      else if (string(argv[i]) == "-h")
      {
         printHelpMessage();
         exit(0);
      }
      else
      {
         printHelpMessage();
         exit(-1);
      }
   }

   _num_cores = (SInt32) Config::getSingleton()->getApplicationCores();
   CarbonBarrierInit(&_global_barrier, _num_cores);

   carbon_thread_t tid_list[_num_cores-1];
   for (SInt32 i = 0; i < _num_cores-1; i++)
   {
      tid_list[i] = CarbonSpawnThread(sendNetworkTraffic, NULL);
   }
   sendNetworkTraffic(NULL);

   for (SInt32 i = 0; i < _num_cores-1; i++)
   {
      CarbonJoinThread(tid_list[i]);
   }
   
   printf("Joined all threads\n");

   Simulator::disablePerformanceModelsInCurrentProcess();

   CarbonStopSim();
 
   return 0;
}

void printHelpMessage()
{
   fprintf(stderr, "[Usage]: ./synthetic_network_traffic_generator -p <arg1> -l <arg2> -s <arg3> -N <arg4>\n");
   fprintf(stderr, "where <arg1> = Network Traffic Pattern Type (uniform_random, bit_complement, shuffle, transpose, tornado, nearest_neighbor) (default uniform_random)\n");
   fprintf(stderr, " and  <arg2> = Number of Packets injected into the Network per Core per Cycle (default 0.1)\n");
   fprintf(stderr, " and  <arg3> = Size of each Packet in Bytes (default 8)\n");
   fprintf(stderr, " and  <arg4> = Total Number of Packets injected into the Network per Core (default 10000)\n");
}


void* sendNetworkTraffic(void*)
{
   // Wait for everyone to be spawned
   CarbonBarrierWait(&_global_barrier);

   Core* core = Sim()->getCoreManager()->getCurrentCore();
   
   int network_traffic_type = UNIFORM_RANDOM;
   vector<int> send_vec;
   vector<int> receive_vec;
   // Generate the Network Traffic
   switch (network_traffic_type)
   {
      case UNIFORM_RANDOM:
         uniformRandomTrafficGenerator(core->getId(), send_vec, receive_vec);
         break;
      case BIT_COMPLEMENT:
         bitComplementTrafficGenerator(core->getId(), send_vec, receive_vec);
         break;
      case SHUFFLE:
         shuffleTrafficGenerator(core->getId(), send_vec, receive_vec);
         break;
      case TRANSPOSE:
         transposeTrafficGenerator(core->getId(), send_vec, receive_vec);
         break;
      case TORNADO:
         tornadoTrafficGenerator(core->getId(), send_vec, receive_vec);
         break;
      case NEAREST_NEIGHBOR:
         nearestNeighborTrafficGenerator(core->getId(), send_vec, receive_vec);
         break;
      default:
         assert(false);
         break;
   }

   Byte data[_packet_size];
   UInt64 outstanding_window_size = 1000;
   RandNum rand_num(0,1);

   for (UInt64 time = 0, total_packets_sent = 0, total_packets_received = 0; 
         (total_packets_sent < _total_packets) || (total_packets_received < _total_packets);
         time ++)
   {
      if ((total_packets_sent < _total_packets) && (canSendPacket(_offered_load, rand_num)))
      {
         // Send a packet to its destination core
         SInt32 receiver = send_vec[total_packets_sent % send_vec.size()];
         NetPacket net_packet(time, _packet_type, core->getId(), receiver, _packet_size, data);
         core->getNetwork()->netSend(net_packet);
         total_packets_sent ++;
      }

      if (total_packets_sent < _total_packets)
      {
         // Synchronize after every few cycles
         synchronize(time, core);
      }

      if ( (total_packets_received < _total_packets) &&
           ( (total_packets_sent == _total_packets) || 
             (total_packets_sent >= (total_packets_received + outstanding_window_size)) ) ) 
      {
         // Check if a packet has arrived for this core (Should be non-blocking)
         NetPacket recv_net_packet = core->getNetwork()->netRecvType(_packet_type);
         delete [] (Byte*) recv_net_packet.data;
         total_packets_received ++;
      }
   }

   // Wait for everyone to finish
   CarbonBarrierWait(&_global_barrier);
   return NULL;
}

bool canSendPacket(double offered_load, RandNum& rand_num)
{
   return (rand_num.next() < offered_load); 
}

void synchronize(UInt64 packet_injection_time, Core* core)
{
   ClockSkewMinimizationClient* clock_skew_client = core->getClockSkewMinimizationClient();

   if (clock_skew_client)
      clock_skew_client->synchronize(packet_injection_time);
}

void computeEMeshTopologyParams(int num_cores, int& mesh_width, int& mesh_height);
void computeEMeshPosition(int core_id, int& sx, int& sy, int mesh_width);
int computeCoreId(int sx, int sy, int mesh_width);

void uniformRandomTrafficGenerator(int core_id, vector<int>& send_vec, vector<int>& receive_vec)
{
   // Generate Random Numbers using Linear Congruential Generator
   int send_matrix[_num_cores][_num_cores];
   int receive_matrix[_num_cores][_num_cores];

   send_matrix[0][0] = _num_cores / 2; // Initial seed
   receive_matrix[0][send_matrix[0][0]] = 0;
   for (int i = 0; i < _num_cores; i++) // Time Slot
   {
      if (i != 0)
      {
         send_matrix[i][0] = send_matrix[i-1][1];
         receive_matrix[i][send_matrix[i][0]] = 0;
      }
      for (int j = 1; j < _num_cores; j++) // Sender
      {
         send_matrix[i][j] = (13 * send_matrix[i][j-1] + 5) % _num_cores;
         receive_matrix[i][send_matrix[i][j]] = j;
      }
   }

   // Check the validity of the random numbers
   for (int i = 0; i < _num_cores; i++) // Time Slot
   {
      vector<bool> bits(_num_cores, false);
      for (int j = 0; j < _num_cores; j++) // Sender
      {
         bits[send_matrix[i][j]] = true;
      }
      for (int j = 0; j < _num_cores; j++)
      {
         assert(bits[j]);
      }
   }

   for (int j = 0; j < _num_cores; j++) // Sender
   {
      vector<bool> bits(_num_cores, false);
      for (int i = 0; i < _num_cores; i++) // Time Slot
      {
         bits[send_matrix[i][j]] = true;
      }
      for (int i = 0; i < _num_cores; i++)
      {
         assert(bits[i]);
      }
   }

   for (SInt32 i = 0; i < _num_cores; i++)
   {
      send_vec.push_back(send_matrix[i][core_id]);
      receive_vec.push_back(receive_matrix[i][core_id]);
   }
}

void bitComplementTrafficGenerator(int core_id, vector<core_id_t>& send_vec, vector<core_id_t>& receive_vec)
{
   assert(isPower2(_num_cores));
   int mask = _num_cores-1;
   int dst_core = (~core_id) & mask;
   send_vec.push_back(dst_core);
   receive_vec.push_back(dst_core);
}

void shuffleTrafficGenerator(int core_id, vector<core_id_t>& send_vec, vector<core_id_t>& receive_vec)
{
   assert(isPower2(_num_cores));
   int mask = _num_cores-1;
   int nbits = floorLog2(_num_cores);
   int dst_core = ((core_id >> (nbits-1)) & 1) | ((core_id << 1) & mask);
   send_vec.push_back(dst_core); 
   receive_vec.push_back(dst_core); 
}

void transposeTrafficGenerator(int core_id, vector<core_id_t>& send_vec, vector<core_id_t>& receive_vec)
{
   int mesh_width, mesh_height;
   computeEMeshTopologyParams(_num_cores, mesh_width, mesh_height);
   int sx, sy;
   computeEMeshPosition(core_id, sx, sy, mesh_width);
   int dst_core = computeCoreId(sy, sx, mesh_width);
   
   send_vec.push_back(dst_core);
   receive_vec.push_back(dst_core);
}

void tornadoTrafficGenerator(int core_id, vector<core_id_t>& send_vec, vector<core_id_t>& receive_vec)
{
   int mesh_width, mesh_height;
   computeEMeshTopologyParams(_num_cores, mesh_width, mesh_height);
   int sx, sy;
   computeEMeshPosition(core_id, sx, sy, mesh_width);
   int dst_core = computeCoreId((sx + mesh_width/2) % mesh_width, (sy + mesh_height/2) % mesh_height, mesh_width);
}

void nearestNeighborTrafficGenerator(int core_id, vector<core_id_t>& send_vec, vector<core_id_t>& receive_vec)
{
   int mesh_width, mesh_height;
   computeEMeshTopologyParams(_num_cores, mesh_width, mesh_height);
   int sx, sy;
   computeEMeshPosition(core_id, sx, sy, mesh_width);
   int dst_core = computeCoreId((sx+1) % mesh_width, (sy+1) % mesh_height, mesh_width);

   send_vec.push_back(dst_core);
   receive_vec.push_back(dst_core);
}

void computeEMeshTopologyParams(int num_cores, int& mesh_width, int& mesh_height)
{
   mesh_width = (int) sqrt((float) num_cores);
   mesh_height = (int) ceil(1.0 * num_cores / mesh_width);
   assert(num_cores == (mesh_width * mesh_height));
}

void computeEMeshPosition(int core_id, int& sx, int& sy, int mesh_width)
{
   sx = core_id % mesh_width;
   sy = core_id / mesh_width;
}

int computeCoreId(int sx, int sy, int mesh_width)
{
   return ((sy * mesh_width) + sx);
}
