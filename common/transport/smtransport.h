#ifndef SMTRANSPORT_H
#define SMTRANSPORT_H

#include <queue>

#include "transport.h"
#include "cond.h"

class SmTransport : public Transport
{
public:
   SmTransport();
   ~SmTransport();

   class SmNode : public Node
   {
   public:
      SmNode(tile_id_t tile_id, SmTransport *smt);
      ~SmNode();

      void globalSend(SInt32, const void*, UInt32);
      void send(tile_id_t, const void*, UInt32);
      Byte* recv();
      bool query();

   private:
      void send(SmNode *dest, const void *buffer, UInt32 length);

      std::queue<Byte*> m_queue;
      Lock m_lock;
      ConditionVariable m_cond;
      SmTransport *m_smt;
   };

   Node* createNode(tile_id_t tile_id);

   void barrier();
   Node* getGlobalNode();

private:
   Node *m_global_node;
   SmNode **m_tile_nodes;

   SmNode *getNodeFromId(tile_id_t tile_id);
   void clearNodeForId(tile_id_t tile_id);
};

#endif
