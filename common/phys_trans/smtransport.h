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
      SmNode(SInt32 core_id, SmTransport *smt);
      ~SmNode();

      void globalSend(SInt32, const void*, UInt32);
      void send(SInt32, const void*, UInt32);
      Byte* recv();
      bool query();

   private:
      void send(SmNode *dest, const void *buffer, UInt32 length);

      std::queue<Byte*> m_queue;
      ConditionVariable m_cond;
      SmTransport *m_smt;
   };

   Node* createNode(SInt32 core_id);

   void barrier();
   Node* getGlobalNode();

private:
   Node *m_global_node;
   SmNode **m_core_nodes;

   SmNode *getNodeFromId(SInt32 core_id);
   void clearNodeForId(SInt32 core_id);
};

#endif
