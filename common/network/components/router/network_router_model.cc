#include "network_router_model.h"
#include "network_model.h"
#include "network.h"
#include "utils.h"

NetworkRouterModel::NetworkRouterModel(NetworkModel* model,
                                       SInt32 num_input_ports, SInt32 num_output_ports,
                                       SInt32 num_flits_per_port_buffer, UInt64 delay, SInt32 flit_width,
                                       bool contention_model_enabled, string& contention_model_type)
   : _model(model)
   , _num_input_ports(num_input_ports)
   , _num_output_ports(num_output_ports)
   , _flit_width(flit_width)
   , _delay(delay)
   , _contention_model_enabled(contention_model_enabled)
{
   if (_contention_model_enabled)
   {
      _contention_model_list.resize(_num_output_ports);
      for (SInt32 i = 0; i < _num_output_ports; i++)
         _contention_model_list[i] = QueueModel::create(contention_model_type, /* UInt64 */ 1);
   }

   initializeEventCounters();
}

NetworkRouterModel::~NetworkRouterModel()
{
   if (_contention_model_enabled)
   {
      for (SInt32 i = 0; i < _num_output_ports; i++)
         delete _contention_model_list[i];
   }
}

void
NetworkRouterModel::initializeEventCounters()
{
   _total_buffer_writes = 0;
   _total_buffer_reads = 0;
   _total_switch_allocator_requests = 0;
   _total_crossbar_traversals.resize(_num_output_ports, 0);
}

void
NetworkRouterModel::processPacket(const NetPacket& pkt, SInt32 output_port,
                                  UInt64& zero_load_delay, UInt64& contention_delay)
{
   vector<SInt32> output_port_list;
   
   if (output_port == OUTPUT_PORT_ALL)
   {
      for (SInt32 i = 0; i < _num_output_ports; i++)
         output_port_list.push_back(i);
   }
   else // only 1 output port
   {
      output_port_list.push_back(output_port);
   }

   processPacket(pkt, output_port_list, zero_load_delay, contention_delay);
}

void
NetworkRouterModel::processPacket(const NetPacket& pkt, vector<SInt32>& output_port_list,
                                  UInt64& zero_load_delay, UInt64& contention_delay)
{
   if (!_model->isModelEnabled(pkt))
      return;

   assert( (1 <= (SInt32) output_port_list.size()) && (_num_output_ports >= (SInt32) output_port_list.size()) );

   // Supposed to increment both zero_load_delay and contention_delay
   SInt32 packet_length = _model->getModeledLength(pkt);
   SInt32 num_flits = _model->computeNumFlits(packet_length);

   // Add to zero_load_delay
   zero_load_delay += _delay;
  
   if (_contention_model_enabled)
   { 
      UInt64 max_queue_delay = 0;
      for (vector<SInt32>::iterator it = output_port_list.begin(); it != output_port_list.end(); it++)
      {
         UInt64 queue_delay = _contention_model_list[*it]->computeQueueDelay(pkt.time, num_flits);
         max_queue_delay = max<UInt64>(max_queue_delay, queue_delay);
      }

      // Add to contention_delay
      contention_delay += max_queue_delay;
   }

   // Increment Event Counters
   _total_buffer_writes += num_flits;
   _total_buffer_reads += num_flits;
   _total_switch_allocator_requests += 1;
   _total_crossbar_traversals[output_port_list.size()-1] += num_flits;
}
