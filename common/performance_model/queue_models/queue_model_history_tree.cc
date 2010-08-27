#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <cassert>

#include "simulator.h"
#include "core_manager.h"
#include "config.h"
#include "queue_model_history_tree.h"
#include "log.h"

#define PAIR(x_,y_)  (make_pair<UInt64,UInt64>(x_,y_))

QueueModelHistoryTree::QueueModelHistoryTree(UInt64 min_processing_time):
   _min_processing_time(min_processing_time),
   _MAX_CYCLE_COUNT(UINT64_MAX)
{
   try
   {
      _max_free_interval_size = Sim()->getCfg()->getInt("queue_model/history_tree/max_list_size");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read queue_model/history_tree parameters from the cfg file");
   }

  
   allocateMemory();

   IntervalTree::Node* start_node = allocateNode(PAIR(0,_MAX_CYCLE_COUNT)); 
   _interval_tree = new IntervalTree(start_node);

   initializeQueueCounters();
}

QueueModelHistoryTree::~QueueModelHistoryTree()
{
   delete _interval_tree;
   releaseMemory();
}

UInt64
QueueModelHistoryTree::computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, core_id_t requester)
{
   LOG_PRINT("Packet(%llu,%llu)", pkt_time, processing_time);
   
   UInt64 queue_delay;

   IntervalTree::Node* node = _interval_tree->search(PAIR(pkt_time, pkt_time + processing_time));
   if (!node)
   {
      _interval_tree->inOrderTraversal();
      LOG_PRINT_ERROR("node = (NULL)");
   }

   assert((pkt_time + processing_time) <= node->interval.second);

   if (pkt_time >= node->interval.first)
   {
      queue_delay = 0;
      if ((pkt_time - node->interval.first) >= _min_processing_time)
      {
         if ((node->interval.second - (pkt_time + processing_time)) >= _min_processing_time)
         {
            IntervalTree::Node* next_node = allocateNode(PAIR(pkt_time + processing_time, node->interval.second));
            _interval_tree->insert(next_node);
         }
         node->interval.second = pkt_time;
      }
      else // ((pkt_time - node->interval.first) < _min_processing_time)
      {
         if ((node->interval.second - (pkt_time + processing_time)) >= _min_processing_time)
         {
            node->interval.first = pkt_time + processing_time;
            node->key = node->interval.first;
         }
         else
         {
            releaseNode(_interval_tree->remove(node));
         }
      }
   }
   else // (pkt_time < node->interval.first)
   {
      queue_delay = node->interval.first - pkt_time;
      if ((node->interval.second - (node->interval.first + processing_time)) >= _min_processing_time)
      {
         node->interval.first = node->interval.first + processing_time;
         node->key = node->interval.first;
      }
      else
      {
         releaseNode(_interval_tree->remove(node));
      }
   }

   IntervalTree::Node* min_node = _interval_tree->search(PAIR(0,1));
   if (min_node->interval.first > pkt_time)
   {
      _total_wrongly_handled_requests ++;
   }
   // Prune the Tree when it grows too large
   if (_interval_tree->size() >= ((UInt32) _max_free_interval_size))
   {
      // Remove the node with the minimum key
      releaseNode(_interval_tree->remove(min_node));
   }

   updateQueueCounters(processing_time);

   LOG_PRINT("Packet(%llu,%llu) -> Queue Delay(%llu)", pkt_time, processing_time, queue_delay);

   return queue_delay;
}

void
QueueModelHistoryTree::initializeQueueCounters()
{
   _total_requests = 0;
   _total_utilized_cycles = 0;
   _total_wrongly_handled_requests = 0;
}

void
QueueModelHistoryTree::updateQueueCounters(UInt64 processing_time)
{
   _total_requests ++;
   _total_utilized_cycles += processing_time;
}

float
QueueModelHistoryTree::getQueueUtilization()
{
   // Get the node with the maximum key
   IntervalTree::Node* node = _interval_tree->search(PAIR(_MAX_CYCLE_COUNT-1,_MAX_CYCLE_COUNT));
   assert(node->interval.second == _MAX_CYCLE_COUNT);

   return ((float) _total_utilized_cycles / node->interval.first);
}

void
QueueModelHistoryTree::allocateMemory()
{
   _memory_blocks = new IntervalTree::Node[_max_free_interval_size];
   _free_memory_block_list = new SInt32[_max_free_interval_size];
   for (SInt32 i = 0; i < _max_free_interval_size; i++)
      _free_memory_block_list[i] = i;
   _free_memory_block_list_tail = _max_free_interval_size - 1;
}

void
QueueModelHistoryTree::releaseMemory()
{
   delete [] _free_memory_block_list;
   delete [] _memory_blocks;
}

IntervalTree::Node*
QueueModelHistoryTree::allocateNode(pair<UInt64,UInt64> interval)
{
   LOG_ASSERT_ERROR(_free_memory_block_list_tail >= 0, \
      "_free_memory_block_list_tail(%i)", _free_memory_block_list_tail);
   
   SInt32 free_index = _free_memory_block_list[_free_memory_block_list_tail];
   IntervalTree::Node* node = &_memory_blocks[free_index];
   node->initialize(PAIR(interval.first, interval.second));
   _free_memory_block_list_tail --;

   return node;
}

void
QueueModelHistoryTree::releaseNode(IntervalTree::Node* node)
{
   SInt32 index = (SInt32) (node - _memory_blocks);
   // LOG_PRINT_WARNING("index(%i)", index);
   assert((index >= 0) && (index < _max_free_interval_size));

   _free_memory_block_list_tail ++;
   LOG_ASSERT_ERROR(_free_memory_block_list_tail < _max_free_interval_size, \
         "_free_memory_block_list_tail(%i)", _free_memory_block_list_tail);
   _free_memory_block_list[_free_memory_block_list_tail] = index;
}
