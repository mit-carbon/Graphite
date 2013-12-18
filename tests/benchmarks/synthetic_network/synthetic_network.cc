#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "network.h"
#include "network_model.h"
#include "clock_skew_management_object.h"
#include "carbon_user.h"
#include "utils.h"
#include "random.h"
#include "log.h"

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
void uniformRandomTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec);
void bitComplementTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec);
void shuffleTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec);
void transposeTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec);
void tornadoTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec);
void nearestNeighborTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec);

bool canSendPacket(double offered_load, Random<double>& rand_num);
void synchronize(Time time, Tile* tile);
void printHelpMessage();
NetworkTrafficType parseTrafficPattern(string traffic_pattern);

NetworkTrafficType _traffic_pattern_type = UNIFORM_RANDOM;     // Network Traffic Pattern Type
double _offered_load = 0.1;                                    // Number of packets injected per tile per cycle
SInt32 _packet_size = 8;                                       // Size of each Packet in Bytes
UInt64 _total_packets = 10000;                                 // Total number of packets injected into the network per tile

PacketType _packet_type = USER;                                // Type of each packet (so as to send on 2nd user network)
carbon_barrier_t _global_barrier;
SInt32 _num_tiles;

int main(int argc, char* argv[])
{
   CarbonStartSim(argc, argv);

   Simulator::enablePerformanceModelsInCurrentProcess();

   // Read Command Line Arguments
   for (SInt32 i = 1; i < argc-1; i += 2)
   {
      if (string(argv[i]) == "-p")
         _traffic_pattern_type = parseTrafficPattern(string(argv[i+1]));
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
         fprintf(stderr, "** ERROR **\n");
         printHelpMessage();
         exit(-1);
      }
   }

   _num_tiles = (SInt32) Config::getSingleton()->getApplicationTiles();
   CarbonBarrierInit(&_global_barrier, _num_tiles);

   carbon_thread_t tid_list[_num_tiles-1];
   for (SInt32 i = 0; i < _num_tiles-1; i++)
   {
      tid_list[i] = CarbonSpawnThread(sendNetworkTraffic, NULL);
   }
   sendNetworkTraffic(NULL);

   for (SInt32 i = 0; i < _num_tiles-1; i++)
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

NetworkTrafficType parseTrafficPattern(string traffic_pattern)
{
   if (traffic_pattern == "uniform_random")
      return UNIFORM_RANDOM;
   else if (traffic_pattern == "bit_complement")
      return BIT_COMPLEMENT;
   else if (traffic_pattern == "shuffle")
      return SHUFFLE;
   else if (traffic_pattern == "transpose")
      return TRANSPOSE;
   else if (traffic_pattern == "tornado")
      return TORNADO;
   else if (traffic_pattern == "nearest_neighbor")
      return NEAREST_NEIGHBOR;
   else
   {
      fprintf(stderr, "** ERROR **\n");
      fprintf(stderr, "Unrecognized Network Traffic Pattern Type (Use uniform_random, bit_complement, shuffle, transpose, tornado, nearest_neighbor)\n");
      exit(-1);
   }
}

void* sendNetworkTraffic(void*)
{
   // Wait for everyone to be spawned
   CarbonBarrierWait(&_global_barrier);

   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   
   vector<tile_id_t> send_vec;
   vector<tile_id_t> receive_vec;
   
   // Generate the Network Traffic
   switch (_traffic_pattern_type)
   {
   case UNIFORM_RANDOM:
      uniformRandomTrafficGenerator(tile->getId(), send_vec, receive_vec);
      break;
   case BIT_COMPLEMENT:
      bitComplementTrafficGenerator(tile->getId(), send_vec, receive_vec);
      break;
   case SHUFFLE:
      shuffleTrafficGenerator(tile->getId(), send_vec, receive_vec);
      break;
   case TRANSPOSE:
      transposeTrafficGenerator(tile->getId(), send_vec, receive_vec);
      break;
   case TORNADO:
      tornadoTrafficGenerator(tile->getId(), send_vec, receive_vec);
      break;
   case NEAREST_NEIGHBOR:
      nearestNeighborTrafficGenerator(tile->getId(), send_vec, receive_vec);
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized traffic pattern (%u)", _traffic_pattern_type);
      break;
   }

   Byte data[_packet_size];
   UInt64 outstanding_window_size = 1000;
   Random<double> rand_num;

   Time time(0);
   for (UInt64 total_packets_sent = 0, total_packets_received = 0; 
         (total_packets_sent < _total_packets) || (total_packets_received < _total_packets); )
   {
      if ((total_packets_sent < _total_packets) && (canSendPacket(_offered_load, rand_num)))
      {
         // Send a packet to its destination tile
         tile_id_t receiver = send_vec[total_packets_sent % send_vec.size()];
         NetPacket net_packet(time, _packet_type, tile->getId(), receiver, _packet_size, data);
         tile->getNetwork()->netSend(net_packet);
         total_packets_sent ++;
      }

      if (total_packets_sent < _total_packets)
      {
         // Synchronize after every few cycles
         synchronize(time, tile);
      }

      if ( (total_packets_received < _total_packets) &&
           ( (total_packets_sent == _total_packets) || 
             (total_packets_sent >= (total_packets_received + outstanding_window_size)) ) ) 
      {
         // Check if a packet has arrived for this core (Should be non-blocking)
         core_id_t core_id = tile->getCore()->getId();
         NetPacket recv_net_packet = tile->getNetwork()->netRecvType(_packet_type, core_id);
         delete [] (Byte*) recv_net_packet.data;
         total_packets_received ++;
      }

      Time ONE_CYCLE = Latency(1, tile->getFrequency());
      time = time + ONE_CYCLE;
   }

   // Wait for everyone to finish
   CarbonBarrierWait(&_global_barrier);
   return NULL;
}

bool canSendPacket(double offered_load, Random<double>& rand_num)
{
   return (rand_num.next(1) < offered_load); 
}

void synchronize(Time packet_injection_time, Tile* tile)
{
   ClockSkewManagementClient* clock_skew_client = tile->getCore()->getClockSkewManagementClient();

   if (clock_skew_client)
      clock_skew_client->synchronize(packet_injection_time);
}

void computeEMeshTopologyParams(int num_tiles, int& mesh_width, int& mesh_height);
void computeEMeshPosition(tile_id_t tile_id, int& sx, int& sy, int mesh_width);
tile_id_t computeTileID(int sx, int sy, int mesh_width);

void uniformRandomTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec)
{
   // Generate Random Numbers using Linear Congruential Generator
   tile_id_t send_matrix[_num_tiles][_num_tiles];
   tile_id_t receive_matrix[_num_tiles][_num_tiles];

   send_matrix[0][0] = _num_tiles / 2; // Initial seed
   receive_matrix[0][send_matrix[0][0]] = 0;
   for (tile_id_t i = 0; i < _num_tiles; i++) // Time Slot
   {
      if (i != 0)
      {
         send_matrix[i][0] = send_matrix[i-1][1];
         receive_matrix[i][send_matrix[i][0]] = 0;
      }
      for (tile_id_t j = 1; j < _num_tiles; j++) // Sender
      {
         send_matrix[i][j] = (13 * send_matrix[i][j-1] + 5) % _num_tiles;
         receive_matrix[i][send_matrix[i][j]] = j;
      }
   }

   // Check the validity of the random numbers
   for (tile_id_t i = 0; i < _num_tiles; i++) // Time Slot
   {
      vector<bool> bits(_num_tiles, false);
      for (tile_id_t j = 0; j < _num_tiles; j++) // Sender
      {
         bits[send_matrix[i][j]] = true;
      }
      for (tile_id_t j = 0; j < _num_tiles; j++)
      {
         assert(bits[j]);
      }
   }

   for (tile_id_t j = 0; j < _num_tiles; j++) // Sender
   {
      vector<bool> bits(_num_tiles, false);
      for (tile_id_t i = 0; i < _num_tiles; i++) // Time Slot
      {
         bits[send_matrix[i][j]] = true;
      }
      for (tile_id_t i = 0; i < _num_tiles; i++)
      {
         assert(bits[i]);
      }
   }

   for (SInt32 i = 0; i < _num_tiles; i++)
   {
      send_vec.push_back(send_matrix[i][tile_id]);
      receive_vec.push_back(receive_matrix[i][tile_id]);
   }
}

void bitComplementTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec)
{
   assert(isPower2(_num_tiles));
   int mask = _num_tiles-1;
   tile_id_t dst_tile = (~tile_id) & mask;
   send_vec.push_back(dst_tile);
   receive_vec.push_back(dst_tile);
}

void shuffleTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec)
{
   assert(isPower2(_num_tiles));
   int mask = _num_tiles-1;
   int nbits = floorLog2(_num_tiles);
   tile_id_t dst_tile = ((tile_id >> (nbits-1)) & 1) | ((tile_id << 1) & mask);
   send_vec.push_back(dst_tile); 
   receive_vec.push_back(dst_tile); 
}

void transposeTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec)
{
   int mesh_width, mesh_height;
   computeEMeshTopologyParams(_num_tiles, mesh_width, mesh_height);
   int sx, sy;
   computeEMeshPosition(tile_id, sx, sy, mesh_width);
   tile_id_t dst_tile = computeTileID(sy, sx, mesh_width);
   
   send_vec.push_back(dst_tile);
   receive_vec.push_back(dst_tile);
}

void tornadoTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec)
{
   int mesh_width, mesh_height;
   computeEMeshTopologyParams(_num_tiles, mesh_width, mesh_height);
   int sx, sy;
   computeEMeshPosition(tile_id, sx, sy, mesh_width);
   tile_id_t dst_tile = computeTileID((sx + mesh_width/2) % mesh_width, (sy + mesh_height/2) % mesh_height, mesh_width);

   send_vec.push_back(dst_tile);
   receive_vec.push_back(dst_tile);
}

void nearestNeighborTrafficGenerator(tile_id_t tile_id, vector<tile_id_t>& send_vec, vector<tile_id_t>& receive_vec)
{
   int mesh_width, mesh_height;
   computeEMeshTopologyParams(_num_tiles, mesh_width, mesh_height);
   int sx, sy;
   computeEMeshPosition(tile_id, sx, sy, mesh_width);
   tile_id_t dst_tile = computeTileID((sx+1) % mesh_width, (sy+1) % mesh_height, mesh_width);

   send_vec.push_back(dst_tile);
   receive_vec.push_back(dst_tile);
}

void computeEMeshTopologyParams(int num_tiles, int& mesh_width, int& mesh_height)
{
   mesh_width = (int) sqrt(1.0 * num_tiles);
   mesh_height = (tile_id_t) ceil(1.0 * num_tiles / mesh_width);
   assert(num_tiles == (mesh_width * mesh_height));
}

void computeEMeshPosition(tile_id_t tile_id, int& sx, int& sy, int mesh_width)
{
   sx = tile_id % mesh_width;
   sy = tile_id / mesh_width;
}

tile_id_t computeTileID(int sx, int sy, int mesh_width)
{
   return ((sy * mesh_width) + sx);
}
