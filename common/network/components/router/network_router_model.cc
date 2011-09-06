#include "network_router_model.h"
#include "network_model.h"
#include "network.h"
#include "utils.h"
#include "queue_model_history_list.h"
#include "queue_model_history_tree.h"
#include "log.h"

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
   initializeContentionCounters();
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

      // Update Contention Counters
      updateContentionCounters(max_queue_delay, output_port_list);
   }

   // Update Event Counters
   updateEventCounters(num_flits, output_port_list);
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
NetworkRouterModel::updateEventCounters(SInt32 num_flits, vector<SInt32>& output_port_list)
{
   // Increment Event Counters
   _total_buffer_writes += num_flits;
   _total_buffer_reads += num_flits;
   _total_switch_allocator_requests += 1;
   _total_crossbar_traversals[output_port_list.size()-1] += num_flits;
}

void
NetworkRouterModel::initializeContentionCounters()
{
   _total_contention_delay.resize(_num_output_ports, 0);
   _total_packets.resize(_num_output_ports, 0);
}

void
NetworkRouterModel::updateContentionCounters(UInt64 contention_delay, vector<SInt32>& output_port_list)
{
   for (vector<SInt32>::iterator it = output_port_list.begin(); it != output_port_list.end(); it++)
   {
      _total_contention_delay[*it] += contention_delay;
      _total_packets[*it] ++;
   }
}

float
NetworkRouterModel::getAverageContentionDelay(SInt32 output_port_start, SInt32 output_port_end)
{
   if (output_port_end == INVALID_PORT)
      output_port_end = output_port_start;

   LOG_ASSERT_ERROR(output_port_end >= output_port_start, "output_port_end(%i) < output_port_start(%i)",
                    output_port_end, output_port_start);

   UInt64 total_contention_delay = 0;
   UInt64 total_packets = 0;
   
   for (SInt32 i = output_port_start; i <= output_port_end; i++)
   {
      total_contention_delay += _total_contention_delay[i];
      total_packets += _total_packets[i];
   }
   
   return ((float) total_contention_delay) / total_packets;
}

float
NetworkRouterModel::getAverageLinkUtilization(SInt32 output_port_start, SInt32 output_port_end)
{
   if (output_port_end == INVALID_PORT)
      output_port_end = output_port_start;
   
   LOG_ASSERT_ERROR(output_port_end >= output_port_start, "output_port_end(%i) < output_port_start(%i)",
                    output_port_end, output_port_start);

   float link_utilization = 0.0;
   for (SInt32 i = output_port_start; i <= output_port_end; i++)
   {
      link_utilization += _contention_model_list[i]->getQueueUtilization();
   }
   link_utilization = link_utilization / (output_port_end - output_port_start + 1);
   return link_utilization;
}

float
NetworkRouterModel::getPercentAnalyticalModelsUsed(SInt32 output_port_start, SInt32 output_port_end)
{
   if (output_port_end == INVALID_PORT)
      output_port_end = output_port_start;

   LOG_ASSERT_ERROR(output_port_end >= output_port_start, "output_port_end(%i) < output_port_start(%i)",
                    output_port_end, output_port_start);
   
   UInt64 total_analytical_model_requests = 0;
   UInt64 total_requests = 0;
   for (SInt32 i = output_port_start; i <= output_port_end; i++)
   {
      assert(_total_packets[i] == _contention_model_list[i]->getTotalRequests());
      total_requests += _total_packets[i];

      QueueModel::Type queue_model_type = _contention_model_list[i]->getType();
      if (queue_model_type == QueueModel::HISTORY_LIST)
      {
         QueueModelHistoryList* queue_model = (QueueModelHistoryList*) _contention_model_list[i];
         total_analytical_model_requests += queue_model->getTotalRequestsUsingAnalyticalModel();
      }
      else if (queue_model_type == QueueModel::HISTORY_TREE)
      {
         QueueModelHistoryTree* queue_model = (QueueModelHistoryTree*) _contention_model_list[i];
         total_analytical_model_requests += queue_model->getTotalRequestsUsingAnalyticalModel();
      }
   }

   return (total_requests > 0) ? (((float) total_analytical_model_requests * 100) / total_requests) : 0.0;
}
