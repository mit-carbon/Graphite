#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "fixed_types.h"

#include <map>

class Transport
{
public:
   virtual ~Transport() { };

   class Node
   {
   public:
      virtual ~Node() { }

      virtual void globalSend(SInt32 dest_proc, Byte *buffer, UInt32 length) = 0;
      virtual void send(SInt32 dest, Byte *buffer, UInt32 length) = 0;
      virtual Byte* recv() = 0;
      virtual bool query() = 0;

   protected:
      SInt32 getCoreId();
      Node(SInt32 core_id);
      
   private:
      SInt32 m_core_id;
   };

   static Transport* create();
   static Transport* getSingleton();

   virtual Node* createNode(SInt32 core_id) = 0;

   virtual void barrier() = 0;
   virtual Node* getGlobalNode() = 0; // for communication not linked to a core

protected:
   Transport();

private:
   static Transport *m_singleton;
};

#endif // TRANSPORT_H
