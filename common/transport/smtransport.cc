#include <string.h>

#include "smtransport.h"
#include "config.h"
#include "log.h"

// -- SmTransport -- //

SmTransport::SmTransport()
{
   LOG_ASSERT_ERROR(Config::getSingleton()->getProcessCount() == 1, "Can only use SmTransport with a single process.");

   Config::getSingleton()->setProcessNum(0);

   m_global_node = new SmNode(-1, this);
   m_tile_nodes = new SmNode* [ Config::getSingleton()->getNumLocalTiles() ];
   for (UInt32 i = 0; i < Config::getSingleton()->getNumLocalTiles(); i++)
      m_tile_nodes[i] = NULL;
}

SmTransport::~SmTransport()
{
   // The networks actually delete the Transport::Nodes, so we
   // shouldn't do it ourselves.
   // for (UInt32 i = 0; i < Config::getSingleton()->getNumLocalTiles(); i++)
   //    delete m_tile_nodes[i];

   delete [] m_tile_nodes;
   delete m_global_node;
}

Transport::Node* SmTransport::createNode(tile_id_t tile_id)
{
   LOG_ASSERT_ERROR((UInt32)tile_id < Config::getSingleton()->getNumLocalTiles(),
                    "Request index out of range: %d", tile_id);
   LOG_ASSERT_ERROR(m_tile_nodes[tile_id] == NULL,
                    "Transport already allocated for id: %d.", tile_id);

   m_tile_nodes[tile_id] = new SmNode(tile_id, this);

   LOG_PRINT("Created node: %p on id: %d", m_tile_nodes[tile_id], tile_id);

   return m_tile_nodes[tile_id];
}

void SmTransport::barrier()
{
   // We assume a single process, so this is a NOOP
}

Transport::Node* SmTransport::getGlobalNode()
{
   return m_global_node;
}

SmTransport::SmNode* SmTransport::getNodeFromId(tile_id_t tile_id)
{
   LOG_ASSERT_ERROR((UInt32)tile_id < Config::getSingleton()->getNumLocalTiles(),
                    "Tile id out of range: %d", tile_id);
   return m_tile_nodes[tile_id];
}

void SmTransport::clearNodeForId(tile_id_t tile_id)
{
   // This is called upon deletion of the node, so we should simply
   // not keep around a dead pointer.
   if ((UInt32)tile_id < Config::getSingleton()->getNumLocalTiles())
      m_tile_nodes[tile_id] = NULL;
}

// -- SmTransportNode -- //

SmTransport::SmNode::SmNode(tile_id_t tile_id, SmTransport *smt)
   : Node(tile_id)
   , m_smt(smt)
{
}

SmTransport::SmNode::~SmNode()
{
   LOG_ASSERT_WARNING(m_queue.empty(), "Unread messages in queue for tile: %d", getTileId());
   m_smt->clearNodeForId(getTileId());
}

void SmTransport::SmNode::globalSend(SInt32 dest_proc, const void *buffer, UInt32 length)
{
   LOG_ASSERT_ERROR(dest_proc == 0, "Destination other than zero: %d", dest_proc);
   send((SmNode*)m_smt->getGlobalNode(), buffer, length);
}

void SmTransport::SmNode::send(SInt32 dest_id, const void* buffer, UInt32 length)
{
   SmNode *dest_node = m_smt->getNodeFromId(dest_id);
   LOG_ASSERT_ERROR(dest_node != NULL, "Attempt to send to non-existent node: %d", dest_id);
   send(dest_node, buffer, length);
}

void SmTransport::SmNode::send(SmNode *dest_node, const void *buffer, UInt32 length)
{
   Byte *data = new Byte[length];
   memcpy(data, buffer, length);

   LOG_PRINT("sending msg -- size: %i, data: %p, dest: %p", length, data, dest_node);

   dest_node->m_lock.acquire();
   dest_node->m_queue.push(data);
   dest_node->m_lock.release();
   dest_node->m_cond.broadcast();
}

Byte* SmTransport::SmNode::recv()
{
   LOG_PRINT("attempting recv -- this: %p", this);

   m_lock.acquire();

   while (true)
   {
      if (!m_queue.empty())
      {
         Byte *data = m_queue.front();
         m_queue.pop();
         m_lock.release();

         LOG_PRINT("msg recv'd -- data: %p, this: %p", data, this);

         return data;
      }
      else
      {
         m_cond.wait(m_lock);
      }
   }
}

bool SmTransport::SmNode::query()
{
   bool result = false;

   m_lock.acquire();
   result = !m_queue.empty();
   m_lock.release();

   return result;
}
