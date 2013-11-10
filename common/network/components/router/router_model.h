#pragma once

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "fixed_types.h"
#include "queue_model.h"
#include "router_power_model.h"

class NetworkModel;
class NetPacket;

class RouterModel
{
public:
   RouterModel(NetworkModel* model, double frequency, double voltage,
               SInt32 num_input_ports, SInt32 num_output_ports,
               SInt32 num_flits_per_port_buffer, UInt64 delay, SInt32 flit_width,
               bool contention_model_enabled, string& contention_model_type);
   ~RouterModel();

   void processPacket(const NetPacket& pkt, SInt32 output_port,
                      UInt64& zero_load_delay, UInt64& contention_delay);
   void processPacket(const NetPacket& pkt, vector<SInt32>& output_port_list,
                      UInt64& zero_load_delay, UInt64& contention_delay);
   
   // Event Counters
   UInt64 getTotalBufferWrites()                            { return _total_buffer_writes; }
   UInt64 getTotalBufferReads()                             { return _total_buffer_reads; }
   UInt64 getTotalSwitchAllocatorRequests()                 { return _total_switch_allocator_requests; }
   UInt64 getTotalCrossbarTraversals(SInt32 multicast_idx)  { return _total_crossbar_traversals[multicast_idx-1]; }

   // Energy Models
   RouterPowerModel* getPowerModel()  { return _power_model; }

   // Contention Counters
   float getAverageContentionDelay(SInt32 output_port_start, SInt32 output_port_end = INVALID_PORT);

   // Link Utilization
   float getAverageLinkUtilization(SInt32 output_port_start, SInt32 output_port_end = INVALID_PORT);
   // Percent Analytical Model Used
   float getPercentAnalyticalModelsUsed(SInt32 output_port_start, SInt32 output_port_end = INVALID_PORT);

   static const SInt32 OUTPUT_PORT_ALL = 0xbabecafe;
   static const SInt32 INVALID_PORT = 0xdeadbeef;

private:
   NetworkModel* _model;
   double _frequency;
   SInt32 _num_input_ports;
   SInt32 _num_output_ports;
   UInt64 _delay;
   bool _contention_model_enabled;
   vector<QueueModel*> _contention_model_list;

   // Event Counters
   UInt64 _total_buffer_writes;
   UInt64 _total_buffer_reads;
   UInt64 _total_switch_allocator_requests;
   vector<UInt64> _total_crossbar_traversals;

   // Energy Model
   RouterPowerModel* _power_model;

   // Contention Counters
   vector<UInt64> _total_contention_delay;
   vector<UInt64> _total_packets;

   // Initialize Event Counters
   void initializeEventCounters();
   // Update Event Counters
   void updateEventCounters(SInt32 num_flits, vector<SInt32>& output_port_list);
   // Initialize Contention Counters
   void initializeContentionCounters();
   // Update Contention Counters
   void updateContentionCounters(UInt64 contention_delay, vector<SInt32>& output_port_list);
};
